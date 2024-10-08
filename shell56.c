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

/* you'll need these includes later: */
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <linux/limits.h>

int builtin_cd(int argc, char **argv)
{
    char *path = NULL;

    if (argc == 1)
    {
        path = getenv("HOME");
    }
    else if (argc == 2)
    {
        path = argv[1];
    }
    else
    {
        fprintf(stderr, "cd: wrong number of arguments\n");
        return 1;
    }

    int status = chdir(path);
    if (status == 1)
    {
        fprintf(stderr, "cd: %s\n", strerror(errno));
    }

    return status;
}

int builtin_pwd(int argc, char **argv)
{
    char buffer[PATH_MAX];
    char *path = getcwd(buffer, sizeof(buffer));
    if (path == NULL)
    {
        fprintf(stderr, "Could not determine present working directory\n");
        return 1;
    }
    printf("%s\n", path);
    fflush(stdout);
    return 0;
}

int builtin_exit(int argc, char **argv)
{
    if (argc == 1)
    {
        exit(0);
    }
    else if (argc == 2)
    {
        exit(atoi(argv[1]));
    }
    else
    {
        fprintf(stderr, "exit: too many arguments\n");
        return 1;
    }
}

int builtex(int argc, char **argv)
{
    pid_t pid = fork(); // create a new process
    if (pid == 0)
    {                            // child process
        signal(SIGINT, SIG_DFL); // restore SIGINT in child processes
        if (execvp(argv[0], argv) == -1)
        {
            fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    else if (pid < 0)
    { // fork failure
        fprintf(stderr, "Fork failed: %s\n", strerror(errno));
        return -1;
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

void handle_special_variable(char **tokens, int n_tokens, int exit_status) // pass a copy of exit_status (pass by value read-only)
{
    char qbuf[16];
    sprintf(qbuf, "%d", exit_status);

    for (int i = 0; i < n_tokens; i++)
    {
        if (strcmp(tokens[i], "$?") == 0)
        {
            tokens[i] = strdup(qbuf); // strdup allocates memory and copies the content of qbuf into that new memory
        }
    }
}

int execute_command(int argc, char **argv, int *exit_status)
{
    char *builtin_commands[] = {"cd", "pwd", "exit"};

    int (*builtin_functions[])(int, char **) = {&builtin_cd, &builtin_pwd, &builtin_exit};

    int status = 1;

    if (argv[0] != NULL)
    {
        for (int i = 0; i < sizeof(builtin_commands) / sizeof(char *); i++)
        {
            *exit_status = (*builtin_functions[i])(argc, argv);
            return *exit_status;
        }
    }

    /*
        TODO: Add code here to create new process for external commands.
        Instead of returning -1, call the function that will create a new process
        executing external commands.
    */
    *exit_status = builtex(argc, argv);
    return *exit_status;
}

int main(int argc, char **argv)
{
    bool interactive = isatty(STDIN_FILENO); /* see: man 3 isatty */
    FILE *fp = stdin;
    int exit_status = 0;

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
            handle_special_variable(tokens, n_tokens, exit_status);
            execute_command(n_tokens, tokens, &exit_status);
        }
    }

    if (interactive)  /* make things pretty */
        printf("\n"); /* try deleting this and then quit with ^D */
}
