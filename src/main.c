#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include "dgcp_handler.h"

#define PORT "1212"


int main(int argc, char** argv)
{
     MY_RN = NULL;                                                              // initialize list of recent neighbors
     MY_PN = NULL;                                                              // initialize list of potential neighbors
     COUNT = 0;                                                                 // initialize data counter
     POS = 0;                                                                   // initialize data positioner

     MY_ID = generate_id();

     int opt, s, b;
     uint16_t myport;
     char *hostname, *hisport;

     if ( argc != 7 || strcmp(argv[1],"-b") || strcmp(argv[3],"-h") || strcmp(argv[5],"-p") )
     {
          fprintf(stderr, "Usage: %s -b 'binding port' -h 'neighbor hostname/IPv4/IPv6' -p 'neighbor port'\n",argv[0] );
          exit(EXIT_FAILURE);
     }

     while((opt = getopt(argc, argv, "b:h:p:")) != -1)
     {
          switch(opt)
          {
               case 'b':
                    myport = atoi(optarg);
                    break;
               case 'h':
                    hostname = optarg;
               case 'p':
                    hisport = optarg;
                    break;
          }
     }

     if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) == -1)
     {
          perror("socket ");
          exit(EXIT_FAILURE);
     }
     struct sockaddr_in6 my_peer6 = {0};
     int psize = sizeof(struct sockaddr_in6);

     my_peer6.sin6_family = AF_INET6;
     my_peer6.sin6_port   = htons(myport);

     if ((b = bind(s, (struct sockaddr*)&my_peer6, psize)) < 0 )
     {
          perror("bind ");
          exit(EXIT_FAILURE);
     }

     struct sockaddr_in6 his_peer6 = {0};
     switch (get_peer_info(hostname, hisport, &s, &his_peer6))                      // get destination peer information
     {
          case -1:
               fprintf(stderr, "Error: could not find host.\n");
               exit(EXIT_FAILURE);
          case -2:
               fprintf(stderr, "Error: failed to create socket.\n");
               exit(EXIT_FAILURE);
     }

     ip_port* key = (ip_port*) calloc(1,sizeof(ip_port));
     memcpy(key->ip, &his_peer6.sin6_addr ,16);
     key->port = his_peer6.sin6_port;
     create_potentiel_neighbor(0,key);

     if ( pthread_mutex_init(&lock, NULL) != 0)
     {
          fprintf(stderr,"mutex init has failed\n");
          exit(EXIT_FAILURE);
     }

     int error = pthread_create(&(tid[0]), NULL, &dgcp_recv, &s);
     if ( error != 0 )
          printf("\nReceive thread can't be created :[%s]", strerror(error));
     error = pthread_create(&(tid[1]), NULL, &routine, &s );
     if ( error != 0 )
          printf("\nRoutine thread can't be created :[%s]", strerror(error));
     error = pthread_create(&(tid[2]), NULL, &data_flood, &s );
     if ( error != 0 )
          printf("\nFlood thread can't be created :[%s]", strerror(error));

     pthread_join(tid[0], NULL);
     pthread_join(tid[1], NULL);
     pthread_join(tid[2], NULL);
     pthread_mutex_destroy(&lock);

     close(s);
     return 0;
}
