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

int extract_commands(int argc, char **argv, char **commands[], int start[], int len[])
{
    int start_idx = 0;
    int command_idx = 0;

    for (int i = 0; i <= argc; i++)
    {
        if ((i == argc) || strcmp(argv[i], "|") == 0)
        {
            argv[i] = NULL;
            commands[command_idx] = &argv[start_idx];
            start[command_idx] = start_idx;
            len[command_idx] = i - start_idx;
            command_idx++;
            start_idx = i + 1;
        }
    }
    return command_idx;
}

void handle_special_variable(char *tokens[], int n_tokens, int exit_status) // pass a copy of exit_status (pass by value read-only)
{
    char qbuf[16];
    sprintf(qbuf, "%d", exit_status);

    for (int i = 0; i < n_tokens; i++)
    {
        if (strcmp(tokens[i], "$?") == 0)
        {
            tokens[i] = qbuf; // strdup allocates memory and copies the content of qbuf into that new memory
            // printf("%s\n", tokens[i]);
        }
    }
}

int exec_pipes(int commandc, char **commands[])
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

            if (execvp(*commands[i], commands[i]) == -1)
            {
                fprintf(stderr, "%s: %s\n", *commands[i], strerror(errno));
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