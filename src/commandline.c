#include <readline/readline.h>
#include <readline/history.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "commandline.h"

char *help[] =                                                                  // command help
{
     "nick 'NICKNAME' : set your nickname",
     "add 'ip' 'port' : add a new neighbor",
     "rm 'ip' 'port' : remove a neighbor",
     "send 'type' 'data' : send data",
     "print 'recent/potential' : print recent/potential neighbor list",
     "verbose '0/1' : OFF/ON verbosemode",
     "leave : leave the chat"
};

char *cmd_name[] =                                                              // command name list
{
     "nick",
     "add",
     "rm",
     "send",
     "print",
     "verbose",
     "leave"
};

int (*cmd_func[]) (char **, int) =                                              // command function list
{
     &cmd_nick,
     &cmd_add,
     &cmd_rm,
     &cmd_send,
     &cmd_print,
     &cmd_verbose,
     &cmd_leave
};

void command_loop(int s)                                                        // main loop
{
     char* prompt;
     char* line;
     char** args;

     initialize_readline ();                                                    // initialize auto completion
     do
     {
          args= (char**) calloc(3,sizeof(char *));
          prompt = "$ ";
          line = readline(prompt);                                             // read commande
          if ( ! line  )                                                        // signal CNTRL+D
               break;                                                           // leave the loop

          if ( !strcmp(line,"") )
               continue;

          call_func(line,args,s);
          free(args);
     } while (1);

     free(args);
     free(line);
     free(prompt);
     exit(EXIT_SUCCESS);
}

int call_func(char* line, char** args, int s)
{
     nb = tokenize(line,args);
     for (int i=0; i<NUM_CMD; i++)
          if ( !strcasecmp(args[0],cmd_name[i]) )
               return cmd_func[i](args,s);
     return -1;

}

int tokenize(char* line,char** args)
{
    int i = 0;
    for (char *token = strtok(line," \t"); token != NULL; token = strtok(NULL, " \t") )
          args[i++] = strdup(token);
    return i;
}

int cmd_nick(char **args, int s)
{
     if ( nb != 2 )
     {
          fprintf(stderr, "Syntax error.\n%s\n",help[0]);
          return -1;
     }
     MY_NICK = strdup(args[1]);
     return 0;
}

int cmd_add(char **args, int s)
{
     if ( nb != 3 )
     {
          fprintf(stderr, "Syntax error.\n%s\n",help[1]);
          return -1;
     }

     struct sockaddr_in6 his_peer6 = {0};
     switch (get_peer_info(args[1], args[2], &s, &his_peer6))                      // get destination peer information
     {
          case -1:
               fprintf(stderr, "Error: could not find host.\n");
          case -2:
               fprintf(stderr, "Error: failed to create socket.\n");
     }

     ip_port* key = (ip_port*) calloc(1,sizeof(ip_port));
     memcpy(key->ip, &his_peer6.sin6_addr ,16);
     key->port = his_peer6.sin6_port;
     create_potentiel_neighbor(0,key);
     return 0;
}

int cmd_rm(char **args, int s)
{
     if ( nb != 3 )
     {
          fprintf(stderr, "Syntax error.\n%s\n",help[2]);
          return -1;
     }

     struct sockaddr_in6 his_peer6 = {0};
     inet_pton(AF_INET6, args[1], &(his_peer6.sin6_addr));
     ip_port* key = (ip_port*) calloc(1,sizeof(ip_port));
     memcpy(key->ip, &his_peer6.sin6_addr ,16);
     key->port = htons(atoi(args[2]));

     if ( search_recent_neighbors_key(MY_RN,key) )
     {
          create_potentiel_neighbor(0,key);
          delete_key_RN(&MY_RN,key);
     }
     else
          fprintf(stderr, "%s %s : doesn't exists in the recent neighbor list\n",args[1], args[2]);

     return 0;
}

int cmd_send(char **args, int s)
{
     char buffer[1024];
     if ( nb < 3 )
     {
          fprintf(stderr, "Syntax error.\n%s\n",help[3]);
          return -1;
     }
     data_key dk = {0};
     uint8_t type = (uint8_t) atoi(args[1]);
     dk.id = MY_ID;
     dk.nonce = (uint32_t) time(NULL);
     recent_neighbors* n = symetric_neighbors();
     sprintf(buffer,"%s: ",MY_NICK);
     if ( n )
     {
          for ( int i=2; i<nb; i++)
               sprintf(buffer,"%s %s",buffer,args[i]);
          add_data(dk,buffer,type,n);
     }
     else
          fprintf(stderr, "Sorry, you don't have friends to talk with...\nNobody loves you =(\n");
     return 0;
}

int cmd_print(char **args, int s)
{
     if ( nb != 2 )
     {
          fprintf(stderr, "Syntax error.\n%s\n",help[4]);
          return -1;
     }

     if ( ! strcasecmp(args[1],"recent") )
          print_recent();
     else if ( ! strcasecmp(args[1],"potential") )
          print_potential();
     else
          fprintf(stderr, "Wrong list.\n");

     return 0;
}

int cmd_verbose(char **args, int s)
{
     if ( nb != 2 )
     {
          fprintf(stderr, "Syntax error.\n%s\n",help[5]);
          return -1;
     }

     verbose = atoi(args[1]);
     return 0;
}

int cmd_leave(char **args, int s)
{
     if ( nb != 1 )
     {
          fprintf(stderr, "Syntax error.\n%s\n",help[6]);
          return -1;
     }

     recent_neighbors* tmp1 = MY_RN;
     while ( tmp1 != NULL )
     {
          ip_port* tmp2 = tmp1->key;
          while ( tmp2 != NULL)
          {
               dgc_packet p2send = {0};
               create_goaway(&p2send,8,1,"Goodbye.");
               dgcp_send(s,tmp2->ip,tmp2->port,p2send);
               tmp2 = tmp2->next;
          }
          tmp1 = tmp1->next;
     }
     exit(EXIT_SUCCESS);
     return 0;
}

int initialize_readline()
{
    rl_attempted_completion_function = fileman_completion;
    return 0;
}

char ** fileman_completion (const char *com, int start, int end)
{
    char **matches;
    matches = (char **)NULL;

    if (start == 0)
        matches = rl_completion_matches (com, command_generator);

    return (matches);
}

char *command_generator (const char *com, int num)
{
    static int indice, len;
    char *completion;

    if (num == 0)
    {
        indice = 0;
        len = strlen(com);
    }

    while (indice < NUM_CMD)
    {
        completion = cmd_name[indice++];

        if (strncmp (completion, com, len) == 0)
            return strdup(completion);
    }

    return NULL;
}
