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
#include <math.h>
#include "dgcp_handler.h"

#define PORT 1212
#define NB_RETRIES 5

recent_neighbors* rn;

void data_flood(int s)
{
     for (int i=0; i<NB_DATA; i++)
     {
          while ( DATA[i].nb < NB_RETRIES )
          {
               potential_neighbors* tmp1 = DATA[i].data_neighbors;
               while ( tmp1 != NULL )
               {
                    ip_port* tmp2 = tmp1->key;
                    while ( tmp2 != NULL )
                    {
                         double t1 = pow(2,DATA[i].nb-1);
                         double t2 = pow(2,DATA[i].nb);
                         int t = (t1 + t2) / 2;
                         sleep(t);
                         dgc_packet p2send = {0};
                         create_data(&p2send,strlen(DATA[i].data),DATA[i].key.id,DATA[i].key.nonce,DATA[i].type,DATA[i].data);
                         dgcp_send(s,tmp2->ip,tmp2->port,p2send);
                         tmp2 = tmp2->next;
                    }
                    tmp1 = tmp1->next;
               }
               DATA[i].nb += 1;
          }
     }
     data_clean(s);
}

void data_clean(int s )
{
     while ( NB_DATA > 0 )
     {
          potential_neighbors* tmp1 = DATA[i].data_neighbors;
          while ( tmp1 != NULL )
          {
               ip_port* tmp2 = tmp1->key;
               while ( tmp2 != NULL )
               {
                    dgc_packet p2send = {0};
                    char msg[50];
                    sprintf(msg,"You didn't ack this data : %d:%d",DATA[0].key.id,DATA[0].key.nonce);
                    create_goaway(&p2send,strlen(msg),2,msg);
                    dgcp_send(s,tmp2->ip,tmp2->port,p2send);
                    tmp2 = tmp2->next;
               }
               tmp1 = tmp1->next;
          }
     }
}

void share_neighbors(int s)
{
     unsigned char dgcp_neighbor[DGCP_SIZE] = {0};
     unsigned char* ptr = dgcp_neighbor;
     ptr[0] =  93;
     ptr[1] = 2;
     uint16_t body_length = 0;
     ptr += 4;
     recent_neighbors* tmp1 = symetric_neighbors();
     ip_port* tmp2;
     while ( tmp1 != NULL && DGCP_SIZE - 4 )
     {
          tmp2 = tmp1->key;
          while ( tmp2 != NULL && body_length < DGCP_SIZE - 4 )
          {
               ptr[0] = 3;
               ptr[1] = 18;
               ptr += 2;
               memcpy(ptr,tmp2->ip,16);
               ptr += 16;
               snprintf((char*)ptr,2,"%d",tmp2->port);
               ptr += 2;
               body_length += 20;
               tmp2 = tmp2->next;
          }
          tmp1 = tmp1->next;
     }
     uint16_t be = htons(body_length);
     snprintf((char*)dgcp_neighbor+2,2,"%d",be);
     dgc_packet p2send = {0};
     memcpy(&p2send, (dgc_packet*) dgcp_neighbor,body_length+4);

     tmp1 = symetric_neighbors();
     while ( tmp1 != NULL )
     {
          tmp2 = tmp1->key;
          while ( tmp2 != NULL )
          {
               dgcp_send(s,tmp2->ip,tmp2->port,p2send);
               tmp2 = tmp2->next;
          }
          tmp1 = tmp1->next;
     }
}


void *routine(void *args)
{
     int *arg = (int*) args;
     int s = *arg;
     recent_neighbors* tmp1;
     ip_port* tmp2;
     int i = 0;
     while (1)
     {
          tmp1 = rn;
          while  ( tmp1 != NULL )
          {
<<<<<<< HEAD
               sleep(30);
=======
               sleep(3);
               i++;
               //if ( i % 4 == 0 )
               //     share_neighbors(s);
>>>>>>> 77892b2f304e5dd43547ab3295ee38792c9ed4d9
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

int main(int argc, char** argv)
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

     rn = (recent_neighbors *) calloc(1,sizeof(recent_neighbors));
     rn->id = 0x677DC6234D2763B5;
     rn->key = (ip_port*) calloc(1,sizeof(ip_port));
     memcpy(rn->key->ip,(char *)&his_peer6.sin6_addr,16);
     rn->key->port = htons(port);
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

     int error = pthread_create(&(tid[0]), NULL, &dgcp_recv, &s);
     if ( error != 0 )
          printf("\nReceive thread can't be created :[%s]", strerror(error));
     error = pthread_create(&(tid[1]), NULL, &routine, &s );
     if ( error != 0 )
          printf("\nRoutine thread can't be created :[%s]", strerror(error));

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
