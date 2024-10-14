/*
 * file:        shell56.c
 * description: skeleton code for simple shell
 *
 * Peter Desnoyers, Northeastern CS5600 Fall 2024
 */

/* <> means don't check the local directory */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/* "" means check the local directory */
#include "parser.h"
#include "builtin_commands.h"
#include "utilities.h"

/* you'll need these includes later: */
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <linux/limits.h>

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

int exec_external_command(int argc, char **argv)
{
    pid_t pid = fork(); // create a new process
    if (pid == 0)
    {                            // child process
        signal(SIGINT, SIG_DFL); // restore SIGINT in child processes

        if (redirect(argv, &argc) == -1)
        { // redirection
            exit(EXIT_FAILURE);
        }
        if (execvp(argv[0], argv) == -1)
        {
            fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    else if (pid < 0)
    { // fork failure
        fprintf(stderr, "Fork failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    else
    { // parent process
        int status;
        do
        {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        return WEXITSTATUS(status);
    }
    return 1;
}

int execute_command(int argc, char **argv)
{
    char *builtin_commands[] = {"cd", "pwd", "exit"};

    int (*builtin_functions[])(int, char **) = {&builtin_cd, &builtin_pwd, &builtin_exit};

    if (argv[0] != NULL)
    {
        for (int i = 0; i < sizeof(builtin_commands) / sizeof(char *); i++)
        {
            if (strcmp(builtin_commands[i], argv[0]) == 0)
            {
                return (*builtin_functions[i])(argc, argv); // return 0 for success, 1 otherwise.
            }
        }

        return exec_external_command(argc, argv);
    }

    return 1;
}

int main(int argc, char **argv)
{
    bool interactive = isatty(STDIN_FILENO); /* see: man 3 isatty */
    FILE *fp = stdin;

    if (argc == 2)
    {
        interactive = false;
        fp = fopen(argv[1], "r");
        if (fp == NULL)
        {
            fprintf(stderr, "%s: %s\n", argv[1], strerror(errno));
            exit(EXIT_FAILURE); /* see: man 3 exit */
        }
    }
    if (argc > 2)
    {
        fprintf(stderr, "%s: too many arguments\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char line[1024], linebuf[1024];
    const int max_tokens = 32;
    char *tokens[max_tokens];
    char qbuf[16];
    int exit_status = 0;
    sprintf(qbuf, "%d", exit_status);

    /* loop:
     *   if interactive: print prompt
     *   read line, break if end of file
     *   tokenize it
     *   print it out <-- your logic goes here
     */
    while (true)
    {
        if (interactive)
        {
            /* print prompt. flush stdout, since normally the tty driver doesn't
             * do this until it sees '\n'
             */
            printf("$ ");
            fflush(stdout);
            signal(SIGINT, SIG_IGN);
        }

        /* see: man 3 fgets (fgets returns NULL on end of file)
         */
        if (!fgets(line, sizeof(line), fp))
            break;

        /* read a line, tokenize it, and print it out
         */
        int n_tokens = parse(line, max_tokens, tokens, linebuf, sizeof(linebuf));

        /* replace the code below with your shell:
         */
        // printf("line:");
        // for (int i = 0; i < n_tokens; i++)
        //     printf(" '%s'", tokens[i]);
        // printf("\n");
        if (n_tokens > 0)
        {
            handle_special_variable(tokens, n_tokens, qbuf);
            if (is_pipe_present(n_tokens, tokens) == 0)
            {
                char **commands[10];
                int start[10];
                int len[10];
                int commandc = extract_commands(n_tokens, tokens, commands, start, len);
                int is_valid = 1;

                for (int i = 0; i < commandc; i++)
                {
                    if (len[i] == 0)
                    {
                        is_valid = 0;
                    }
                }
                if (is_valid == 1)
                {
                    exit_status = exec_pipes(commandc, commands);
                }
            }
            else
            {
                exit_status = execute_command(n_tokens, tokens);
            }
            sprintf(qbuf, "%d", exit_status);
        }
    }

    if (interactive)  /* make things pretty */
        printf("\n"); /* try deleting this and then quit with ^D */
}
