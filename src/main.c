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
#include <fcntl.h>
#include "dgcp_handler.h"

#define PORT 1212


int main()
{
     int s,b;
     struct sockaddr_in6 my_peer6 = {0};
     int psize = sizeof(struct sockaddr_in6);

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

     dgc_packet p2send = {0};
     unsigned char ipv6[20] = "::ffff:81.194.27.155";
     int port = 1212;

     int error = pthread_create(&(tid[0]), NULL, &dgcp_recv, &s);
     if ( error != 0 )
          printf("\nReceive thread can't be created :[%s]", strerror(error));

     //create_pad1(&p2send);
     //create_padN(&p2send,10);
     create_short_hello(&p2send);
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



     dgcp_send(s,ipv6,port,p2send);

     pthread_join(tid[0], NULL);
     pthread_mutex_destroy(&lock);

     close(s);
     return 0;
}
