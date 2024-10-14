#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/* you'll need these includes later: */
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <linux/limits.h>

/**
 * Checks for the presence of pipes.
 */
int is_pipe_present(int n_tokens, char *tokens[])
{
    for (int i = 0; i < n_tokens; i++)
    {
        if (strcmp(tokens[i], "|") == 0)
        {
            // tokens[i] = NULL;
            return 0;
        }
    }

    return 1; // return 0 if pipes present, otherwise return 1.
}

int extract_commands(int argc, char **argv, int start[], int end[])
{
    int start_idx = 0;
    int command_idx = 0;

    for (int i = 0; i <= argc; i++)
    {
        if ((i == argc) || strcmp(argv[i], "|") == 0)
        {
            argv[i] = NULL;
            start[command_idx] = start_idx;
            end[command_idx] = i; // includes the NULL token.
            command_idx++;
            start_idx = i + 1;
        }
    }
    return command_idx;
}

void handle_special_variable(char *tokens[], int n_tokens, char qbuf[]) // pass a copy of exit_status (pass by value read-only)
{
    for (int i = 0; i < n_tokens; i++)
    {
        if (strcmp(tokens[i], "$?") == 0)
        {
            tokens[i] = qbuf; // strdup allocates memory and copies the content of qbuf into that new memory
            // printf("%s\n", tokens[i]);
        }
    }
}

int redirect(char **argv, int *argc)
{
    int input_fd = -1, output_fd = -1; // initialize two variables to store file descriptors for input and output redirection

    for (int i = 0; i < *argc; i++)
    {
        if (strcmp(argv[i], "<") == 0) // input redirection
        {

            if (i + 1 < *argc)
            {
                input_fd = open(argv[i + 1], O_RDONLY); // O_RDONLY: read-only flag
                if (input_fd == -1)
                {
                    fprintf(stderr, "Unable to open %s: %s\n", argv[i + 1], strerror(errno));
                    return -1;
                }
                dup2(input_fd, STDIN_FILENO); // duplicate input_fd to standard input, redirecting input from the file
                close(input_fd);

                // remove the < and the filename from argv
                for (int j = i; j < *argc - 2; j++)
                {
                    argv[j] = argv[j + 2];
                }
                *argc -= 2;
                i--;
            }
            else
            {
                fprintf(stderr, "Expected filename after '<'\n");
                return -1;
            }
        }
        else if (strcmp(argv[i], ">") == 0) // output redirection
        {

            if (i + 1 < *argc)
            {
                /**
                 * O_WRONLY: Opens the file in write-only mode
                 * O_CREAT: If the file does not exist, create the file
                 * O_TRUNC: If the file already exists, truncates it (clears its content) to zero length before writing new data
                 * 0644: file permission mode required for setting file permissions on new files, provides write access to the owner and read access to others
                 */
                output_fd = open(argv[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (output_fd == -1)
                {

                    fprintf(stderr, "Unable to open file %s: %s \n", argv[i + 1], strerror(errno));
                    return -1;
                }
                dup2(output_fd, STDOUT_FILENO); // duplicate output_fd to standard output, redirecting output to the file
                close(output_fd);

                for (int j = i; j < *argc - 2; j++)
                {
                    argv[j] = argv[j + 2];
                }
                *argc -= 2;
                i--;
            }
            else
            {
                fprintf(stderr, "Expected filename after '>'\n");
                return -1;
            }
        }
    }

    argv[*argc] = NULL;
    return 0;
}

int exec_pipes(int commandc, char **argv, int start[], int end[])
{
    int status;
    int pipes_count = commandc - 1;
    int fd[pipes_count * 2];

    for (int i = 0; i < pipes_count; i++)
    {
        if (pipe((fd + i * 2)) < 0)
        {
            fprintf(stderr, "pipe failure: %s\n", strerror(errno));
        }
    }

    pid_t pids[commandc];
    int i = 0, j = 0;

    while (i < commandc)
    {
        pids[i] = fork();

        if (pids[i] == 0)
        {
            if (i != commandc - 1) // not the last command
            {
                dup2(fd[j + 1], 1);
            }

            if (j != 0) // not the first command
            {
                dup2(fd[j - 2], 0);
            }

            for (int i = 0; i < pipes_count * 2; i++)
            {
                close(fd[i]);
            }

            int argc = end[i] - start[i];
            // int *argc_ptr = &argc;
            if (redirect(&argv[start[i]], &argc) < 0)
            {
                exit(EXIT_FAILURE);
            }

            if (execvp(argv[start[i]], &argv[start[i]]) == -1)
            {
                fprintf(stderr, "%s: %s\n", argv[start[i]], strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
        else if (pids[i] < 0)
        {
            fprintf(stderr, "Fork failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        j += 2;
        i++;
    }

    for (int i = 0; i < pipes_count * 2; i++)
    {
        close(fd[i]);
    }

    for (int i = 0; i < commandc; i++)
    {
        do
        {
            waitpid(pids[i], &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        status = WEXITSTATUS(status);
    }

    return status;
}