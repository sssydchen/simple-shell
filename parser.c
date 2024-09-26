/*
 * file:        parser.c
 * description: skeleton code for simple shell
 *
 * Peter Desnoyers, Northeastern CS5600 Fall 2023
 */

#include <stdio.h>
#include <ctype.h>

#include "parser.h"

/* makes a bit of logic below prettier */
int isquote(int c)
{
    return c == '"' || c == '\'';
}

/* really simple command line parser.
 * probably fails in a lot of corner cases.
 */

/* tokenizer, takes previous char c1 (=0 at start) and current char c2.
 * returns the OR of the following values:
 *   SPLIT - c2 goes in a new word
 *   SAVE - c2 gets stored in its word
 * it's horribly ad-hoc, and the approach fails for more complex grammars
 * (e.g. it can't do wildcards right without extensive modification)
 */
static int in_2quote;
static int in_1quote;
int split(char c1, char c2)
{
    if (c1 == 0) 
        return NO_SPLIT | (isspace(c2) ? NO_SAVE : SAVE);
    if (in_2quote) {
        if (c2 == '"') {
            in_2quote = 0;
            return SPLIT | NO_SAVE;
        }
        else
            return NO_SPLIT | SAVE;
    }
    else if (in_1quote) {
        if (c2 == '\'') {
            in_1quote = 0;
            return SPLIT | NO_SAVE;
        }
        else
            return NO_SPLIT | SAVE;
    }
    if (c2 == '"') {
        in_2quote = 1;
        return (isspace(c1) ? NO_SPLIT : SPLIT) | NO_SAVE;
    }
    if (c2 == '\'') {
        in_1quote = 1;
        return (isspace(c1) ? NO_SPLIT : SPLIT) | NO_SAVE;
    }
    if (c2 == '|')
	return (isspace(c1) ? NO_SPLIT : SPLIT) | SAVE;
    if (c1 == '|') 
	return SPLIT | (isspace(c2) ? NO_SAVE : SAVE);
    if (isspace(c2))
        return ((isspace(c1) || isquote(c1)) ? NO_SPLIT : SPLIT) | NO_SAVE;
    if (c2 == '>' || c2 == '<')
        return (isspace(c1) ? NO_SPLIT : SPLIT) | SAVE;
    if (c1 == '>' || c1 == '<')
        return SPLIT | (isspace(c2) ? NO_SAVE : SAVE);
    return NO_SPLIT | SAVE;
}

/* parse an input line, copying individual words (plus terminating
 * null characters) into an output buffer, and storing pointers to 
 * those words in an argv-like array, both passed by the caller.
 *
 * returns: number of words in argv
 */
int parse(const char *line, int argc_max, char **argv, char *buf, int buf_len)
{
    in_1quote = in_2quote = 0;
    char *ptr = buf;
    int i = 0, prev = 0;
    argv[i] = ptr;
    for (const char *p = line; *p != 0; p++) {
	int val = split(prev, *p);
	if (val & SPLIT) {
	    *ptr++ = 0;
	    argv[++i] = ptr;
	}
	if (val & SAVE)
	    *ptr++ = *p;
	prev = *p;
	if (ptr > buf+buf_len-2)
	    break;
	if (i >= argc_max-1)
	    break;
    }
    if (ptr != argv[i]) { /* in word at line end? */
	*ptr++ = 0;
	i++;
    }
    argv[i] = NULL;
    return i;
}

#ifdef TEST
#include <stdio.h>
int main(int argc, char **argv)
{
    FILE *fp = stdin;
    if (argc > 1)
	fp = fopen(argv[1], "r");
    char line[1024], buf[1024], *words[20];
    while (fgets(line, sizeof(line), fp)) {
	const char *comma = "";
	int n = parse(line, 20, words, buf, sizeof(buf));
	printf("[");
	for (int i = 0; i < n; i++) {
	    printf("%s'%s'", comma, words[i]);
	    comma = ", ";
	}
	printf("]\n");
    }
}
#endif

