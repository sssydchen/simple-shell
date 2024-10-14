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

    int res = chdir(path);
    if (res < 0)
    {
        fprintf(stderr, "cd: %s\n", strerror(errno));
        return 1;
    }

    return 0;
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