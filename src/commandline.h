#ifndef COMMANDLINE_H_INCLUDED
#define COMMANDLINE_H_INCLUDED
#include <stdio.h>
#include "dgcp_handler.h"
#include "neighborsController.h"

#define NUM_CMD 7

int nb;

void command_loop(int );
int call_func(char* line, char** args, int s);
int tokenize(char* line,char** args);

int cmd_nick(char **, int s);
int cmd_add(char **, int s);
int cmd_rm(char **, int s);
int cmd_send(char **, int s);
int cmd_print(char **, int s);
int cmd_verbose(char **, int s);
int cmd_leave(char **, int s);

int initialize_readline();
char ** fileman_completion (const char *, int , int );
char *command_generator (const char *, int );

#endif
