#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "hexdump.h"
#include "header.h"

#define PORT_STR "1212"
#define PORT_INT  1212



int get_peer_info(char* hostname, int *sock, struct sockaddr_in6 *addr)
{
     struct addrinfo hints, *r, *p;

     memset(&hints, 0, sizeof(hints));

     hints.ai_family = AF_INET6;
     hints.ai_socktype = SOCK_DGRAM;
     hints.ai_flags = (AI_V4MAPPED | AI_ALL);

     if ((getaddrinfo(hostname, PORT_STR, &hints, &r)) != 0 || NULL == r)
          return -1;

     for (
          p = r;
          (NULL != p && (*sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0);
          p = p->ai_next
         );

     if (NULL == p)
          return -2;

     *addr = *((struct sockaddr_in6*) p->ai_addr);
     freeaddrinfo(r);

     return 0;
}


int main()
{
     int s,b;

     char* hostname = "jch.irif.fr";

     struct sockaddr_in6 my_peer6 = {0};
     struct sockaddr_in6 his_peer6 = {0};

     int psize = sizeof(struct sockaddr_in6);

     MY_ID = generate_id();

     if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) == -1)
     {
          perror("socket ");
          exit(EXIT_FAILURE);
     }

     my_peer6.sin6_family = AF_INET6;
     my_peer6.sin6_port = htons(PORT_INT);

     if ((b = bind(s, (struct sockaddr*)&my_peer6, psize)) < 0 )
     {
          perror("bind ");
          exit(EXIT_FAILURE);
     }

     switch (get_peer_info(hostname, &s, &his_peer6))
     {
          case -1:
               fprintf(stderr, "Error: could not find host.\n");
               exit(EXIT_FAILURE);
          case -2:
               fprintf(stderr, "Error: failed to create socket.\n");
               exit(EXIT_FAILURE);
     }

     msg_packet my_msg = {0};
     msg_packet his_msg = {0};

     create_long_hello(&my_msg,dst_id);
     int size = my_msg.header.body_length + 4;
     socklen_t addrlen = sizeof(his_peer6);

     while (1)
     {
          if ( sendto(s, (char*) &my_msg, size, 0, (struct sockaddr *) &his_peer6, addrlen) <0 )
          {
               perror("sendto ");
               close(s);
               exit(EXIT_FAILURE);
          }
          printf("Sent:\n");
          hexdump((char*) &my_msg,size);

          if (recvfrom(s, (char*) &his_msg, MSG_SIZE , 0,  (struct sockaddr *)  &his_peer6, &addrlen) < 0)
          {
               perror("recvfrom ");
               close(s);
               exit(EXIT_FAILURE);
          }

          printf("Received:\n");
          hexdump(&his_msg,his_msg.header.body_length+4);

          break;
     }


     close(s);
     return 0;
}
