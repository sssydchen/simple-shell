/*
 * file:        parser.c
 * description: skeleton code for simple shell
 *
 * Peter Desnoyers, Northeastern CS5600 Fall 2023
 */

/* standard include file protection: 
*/
#ifndef __PARSER_H__
#define __PARSER_H__

/* types: 
*/
enum {
    NO_SPLIT = 0,
    SPLIT = 1,
    NO_SAVE = 0,
    SAVE = 2
};

/* function declarations: 
*/
int split(char c1, char c2);
int parse(const char *line, int argc_max, char **argv, char *buf, int buf_len);

#endif

