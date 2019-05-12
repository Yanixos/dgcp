#ifndef COMMANDLINE_H_INCLUDED
#define COMMANDLINE_H_INCLUDED
#include <stdio.h>
#include "dgcp_handler.h"
#include "neighborsController.h"

#define NUM_CMD 7

int nb;                                                                         // nombre d'argc pour chaque commande

void command_loop(int );                                                        // boucle principale
int call_func(char* line, char** args, int s);                                  // appel la fonction qui convient
int tokenize(char* line,char** args);                                           // split command line into tokens

int cmd_nick(char **, int s);                                                   // change nickname
int cmd_add(char **, int s);                                                    // add neighbor
int cmd_rm(char **, int s);                                                     // remove neighbor
int cmd_send(char **, int s);                                                   // send data
int cmd_print(char **, int s);                                                  // print neighbor list
int cmd_verbose(char **, int s);                                                // (en/dis)able verbose mode
int cmd_leave(char **, int s);                                                  // leave the chat groupe

int initialize_readline();                                                      // function for auto completion
char ** fileman_completion (const char *, int , int );
char *command_generator (const char *, int );

#endif
