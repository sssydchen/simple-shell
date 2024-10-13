#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <linux/limits.h>

/**
 * Updates the tokens which are equal to "|" to NULL and returns the total pipes present.
 */
int check_and_update_pipes(int n_tokens, char *tokens[])
{
    int pipes_present = 0;

    for (int i = 0; i < n_tokens; i++)
    {
        if (strcmp(tokens[i], "|") == 0)
        {
            tokens[i] = NULL;
            pipes_present++;
        }
    }

    return pipes_present; // return 0 if pipes present, otherwise return 1.
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