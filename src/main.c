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

#define PORT 1212

typedef struct
{
     int s;
     recent_neighbors* rn;
} arguments;

void *hello_refresher(void *args)
{
     arguments* arg = args;
     recent_neighbors* tmp1;
     ip_port* tmp2;
     int s = arg->s;
     while (1)
     {
          tmp1 = arg->rn;
          while  ( tmp1 != NULL )
          {
               sleep(30);
               tmp2 = tmp1->key;
               while ( tmp2 )
               {
                    dgc_packet p2send = {0};
                    create_long_hello(&p2send,tmp1->id);
                    dgcp_send(s,tmp2->ip,tmp2->port,p2send);
                    tmp2 = tmp2->next;
               }
               tmp1 = tmp1->next;
          }
     }
}

int main()
{
     int s,b;
     struct sockaddr_in6 my_peer6 = {0};
     int psize = sizeof(struct sockaddr_in6);

     struct sockaddr_in6 his_peer6 = {0};

     char ipv6[20] = "::ffff:81.194.27.155";
     int port = 1212;

     his_peer6.sin6_family = AF_INET6;
     his_peer6.sin6_port = htons(port);

     if(inet_pton(AF_INET6, ipv6, &his_peer6.sin6_addr)<=0)
     {
          fprintf(stderr,"inet_pton has failed\n");
          exit(EXIT_FAILURE);
     }

     recent_neighbors* rn = (recent_neighbors *) calloc(1,sizeof(recent_neighbors));
     rn->id = 0x677DC6234D2763B5;
     rn->key = (ip_port*) calloc(1,sizeof(ip_port));
     memcpy(rn->key->ip,(char *)&his_peer6.sin6_addr,16);
     rn->key->port = port;
     rn->key->next = NULL;
     rn->symetric = 1;
     rn->hello_t = 0;
     rn->long_hello_t = 0;
     rn->next = NULL;

     MY_ID = generate_id();

     if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) == -1)
     {
          perror("socket ");
          exit(EXIT_FAILURE);
     }


     my_peer6.sin6_family = AF_INET6;
     my_peer6.sin6_port   = htons(PORT);

     if ((b = bind(s, (struct sockaddr*)&my_peer6, psize)) < 0 )
     {
          perror("bind ");
          exit(EXIT_FAILURE);
     }

     if ( pthread_mutex_init(&lock, NULL) != 0)
     {
          fprintf(stderr,"mutex init has failed\n");
          exit(EXIT_FAILURE);
     }

     arguments args = {s,rn};
     int error = pthread_create(&(tid[0]), NULL, &dgcp_recv, &s);
     if ( error != 0 )
          printf("\nReceive thread can't be created :[%s]", strerror(error));
     error = pthread_create(&(tid[1]), NULL, &hello_refresher, (void*)&args);
     if ( error != 0 )
          printf("\nRefresh thread can't be created :[%s]", strerror(error));

     //dgc_packet p2send = {0};
     //create_pad1(&p2send);
     //create_padN(&p2send,10);
     //create_short_hello(&p2send);
     //uint64_t id = 0x1122334455667788;
     //create_long_hello(&p2send,id);
     //unsigned char ip[16] = "1122334455667788";
     //uint8_t p = 100;
     //create_neighbor(&p2send,ip,p);
     //uint32_t nonce = 0x12345678;
     //create_ack(&p2send,id,nonce);
     //char msg[] = "testing warning";
     //uint8_t code = 3;
     //create_goaway(&p2send,strlen(msg),code,msg);
     //create_warning(&p2send,strlen(msg),msg);
     //dgcp_send(s,ip,port,p2send);

     pthread_join(tid[0], NULL);
     pthread_join(tid[1], NULL);
     pthread_mutex_destroy(&lock);

     close(s);
     return 0;
}
