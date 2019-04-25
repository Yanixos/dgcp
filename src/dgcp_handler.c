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
#include "hexdump.h"
#include "dgcp_handler.h"

uint64_t his_id;


void dgcp_send(int s, unsigned char ipv6[], int port, dgc_packet p2send)
{
     struct sockaddr_in6 his_peer6 = {0};

     his_peer6.sin6_family = AF_INET6;
     his_peer6.sin6_port = htons(port);

     if(inet_pton(AF_INET6, (char *)ipv6, &his_peer6.sin6_addr)<=0)
     {
          fprintf(stderr,"inet_pton has failed\n");
          exit(EXIT_FAILURE);
     }

     socklen_t addrlen = sizeof(his_peer6);
     int size = p2send.header.body_length + 4;

     if ( sendto(s, (char*) &p2send, size, 0, (struct sockaddr *) &his_peer6, addrlen) < 0 )
     {
          perror("sendto ");
          close(s);
          exit(EXIT_FAILURE);
     }
     hexdump((char*) &p2send.tlv, p2send.tlv.padn.length+2,"Sent");
}

void *dgcp_recv(void *arguments)
{
     pthread_mutex_lock(&lock);

     int *arg = (int*) arguments;
     int s = *arg;

     struct sockaddr_in6 peer6 = {0};
     socklen_t addrlen = sizeof(peer6);
     dgc_packet p2recv = {0};

     unsigned char ipv6[20] = "::ffff:81.194.27.155";
     int port = 1212;

     uint32_t nonce = generate_nonce();
     uint8_t data_type = 0;
     char msg[] = "N1x0s : This project rocks";
     int l = strlen(msg)+13;
     dgc_packet p2send = {0};

     fd_set read_fds;
     fd_set write_fds;
     int i =1;
     int rc = fcntl(s, F_GETFL);
     if ( rc < 0 )
     {
          perror("fcntl F_GETFL ");
          exit(EXIT_FAILURE);
     }
     rc = fcntl(s, F_SETFL, rc | O_NONBLOCK);
     if ( rc < 0 )
     {
          perror("fcntl F_SETFL ");
          exit(EXIT_FAILURE);
     }

     while (1)
     {
          FD_ZERO(&read_fds);
          FD_SET(s, &read_fds);
          int n = select(s+1, &read_fds, 0, 0, 0);
          if(n < 0)
          {
               perror("select ");
               close(s);
               exit(EXIT_FAILURE);
          }
          if(FD_ISSET(s, &read_fds))
          {
               if ( recvfrom(s, (char*) &p2recv, DGCP_SIZE , 0,  (struct sockaddr *)  &peer6, &addrlen) < 0 )
               {
                    perror("recvfrom ");
                    close(s);
                    exit(EXIT_FAILURE);
               }
               FD_CLR(s, &read_fds);
               getpeername(s, (struct sockaddr *) &peer6, &addrlen);
               char str[INET6_ADDRSTRLEN];
               if(inet_ntop(AF_INET6, &peer6.sin6_addr, str, sizeof(str)))
                     printf("connection from %s at %d\n", str, ntohs(peer6.sin6_port));
               printf("WHOLE PACKET:\n");
               hexdump((char*) &p2recv.tlv, p2recv.header.body_length+4,"Received");
               // check if exists in neighbor_list peer6.sin6_addr:peer6.sin6_port

               his_id = p2recv.tlv.l_hello.src_id;

               FD_ZERO(&write_fds);
			FD_SET(s, &write_fds);
			if(FD_ISSET(s, &write_fds))
               {
                    header_handler(s,ipv6,port,p2recv);

                    if ( i == 1 )
                         create_long_hello(&p2send, his_id);
                    else if ( i == 2 )
                         create_data(&p2send,l,MY_ID,nonce,data_type,msg);
                    else
                         break;
                    dgcp_send(s,ipv6,port,p2send);

				FD_CLR(s, &write_fds);
               }
               i++;
          }
     }
     pthread_mutex_unlock(&lock);
     pthread_exit(NULL);
     return NULL;
}

void header_handler(int s, unsigned char ipv6[], int port, dgc_packet p2recv)
{
     unsigned char buff[DGCP_SIZE];
     memcpy(buff, (const unsigned char*) &p2recv, sizeof(p2recv));
     uint16_t body_length = p2recv.header.body_length;
     uint8_t type = buff[4];

     dgc_packet response = {0};

     if ( ! check_header(&p2recv) )
     {
          printf("Non dgcp packet\n");
          char msg[] = "Your are not talking dgcp";
          uint8_t code = 3;
          create_goaway(&response,strlen(msg)+1,code,msg);
          dgcp_send(s,ipv6,port,response);
          return ;
     }

     memmove(buff, buff+4, DGCP_SIZE-4);

     if ( type < 0 || type > 7 )
     {
          printf("Unknown tlv type\n");
          char msg[] = "Your tlv type is unknown";
          uint8_t code = 3;
          create_goaway(&response,strlen(msg)+1,code,msg);
          dgcp_send(s,ipv6,port,response);
          return ;
     }

     if ( body_length > DGCP_SIZE )
     {
          printf("Packet size is larger than the PMTU\n");
          char msg[] = "Your packet is too large, extradata will be ignored";
          create_warning(&response,strlen(msg),msg);
          dgcp_send(s,ipv6,port,response);
     }

     int8_t tlv_length = ( buff[1] > 0 ? buff[1] :  -1 ) ;
     msg_body tlv;
     unsigned char* tmp = buff;

     do
     {
          memset(&tlv,0x00,sizeof(msg_body));
          memcpy(&tlv, (msg_body*) tmp,tlv_length+2);
          type = tmp[0];
          call_tlv_handler(s,ipv6,port,tlv,type);
          if ( type == 0 )
          {
               body_length -= 1;
               tmp += 1;
          }
          else
          {
               body_length -= ( tlv_length + 2);
               tmp += tlv_length + 2;
          }
          tlv_length = tmp[1];
     } while ( body_length > tlv_length + 1 );
}


void call_tlv_handler(int s, unsigned char ipv6[], int port, msg_body tlv, uint8_t type)
{
     switch ( type )
     {
          case 0 :
               hexdump((char *)&tlv,1,"Received");
               break;
          case 1 :
               hexdump((char *)&tlv,tlv.padn.length,"Received");
               break;
          case 2 :
               hello_handler(s,ipv6,port,tlv);
               break;
          case 3 :
               neighbor_handler(s,ipv6,port,tlv);
               break;
          case 4 :
               data_handler(s,ipv6,port,tlv);
               break;
          case 5 :
               ack_handler(s,ipv6,port,tlv);
               break;
          case 6 :
               goaway_handler(s,ipv6,port,tlv);
               break;
          case 7 :
               printf("%s sent a warning : %s\n",ipv6,tlv.warning.message);
               break;
     }
}

void hello_handler(int s, unsigned char ipv6[], int port, msg_body tlv)
{
     dgc_packet response = {0};
     if ( tlv.s_hello.length == 8 )
     {
          hexdump((char *)&tlv,tlv.s_hello.length+2,"Received");
          // add to potential neighbor list
     }
     else if ( tlv.s_hello.length == 16 )
     {
          hexdump((char *)&tlv,tlv.l_hello.length+2,"Received");
          if ( tlv.l_hello.dst_id != MY_ID )
          {
               printf("The destination id is wrong\n");
               char msg[] = "Misdirection hello (that's not my id)";
               create_warning(&response,strlen(msg),msg);
               dgcp_send(s,ipv6,port,response);
          }
          else
          {
               his_id = tlv.l_hello.src_id;          // to remove *****************************
               create_long_hello(&response,his_id);         //
               dgcp_send(s,ipv6,port,response);             //
               // add to symetric neighbor list
          }
     }
     else
     {
          printf("Received corrupted hello tlv\n");
          char msg[] = "Your hello tlv has a bad size";
          uint8_t code = 3;
          create_goaway(&response,strlen(msg)+1,code,msg);
          dgcp_send(s,ipv6,port,response);
     }
}

void neighbor_handler(int s, unsigned char ipv6[], int port, msg_body tlv)
{
     dgc_packet response = {0};

     if ( tlv.neighbor.length == 18 )
     {
          hexdump((char *)&tlv,tlv.neighbor.length+2,"Received");

          // add to potential neighbor list
          // if the one who sent the packet is my neighbor addd his neighbor to my potential list
     }
     else
     {
          printf("Received corrupted neighbor tlv\n");
          char msg[] = "Your neighbor tlv has a bad size";
          create_warning(&response,strlen(msg),msg);
          dgcp_send(s,ipv6,port,response);
     }
}

void data_handler(int s, unsigned char ipv6[], int port, msg_body tlv)
{
     dgc_packet response = {0};

     hexdump((char *)&tlv,tlv.data.length+2,"Received");
     /*
     if the neighbor is not in my neighbor list send a warning saying you sent me data but you are nt my data
     printf("Received ack from a stranger\n");
     char msg[] = "You sent Data but you are not symetric";
     create_warning(&response,strlen(msg),msg);
     dgcp_send(s,ipv6,port,response);
     */
     if ( tlv.data.data_type == 0 )
          printf("%s\n",tlv.data.message);
     create_ack(&response,tlv.data.sender_id,tlv.data.nonce);
     dgcp_send(s,ipv6,port,response);

     // forward the msg to my neighbors
}

void ack_handler(int s, unsigned char ipv6[], int port, msg_body tlv)
{
     dgc_packet response = {0};

     if ( tlv.ack.length == 12  )
     {
          hexdump((char *)&tlv,tlv.ack.length+2,"Received");
          /*
          if the neighbor is not in my neighbor list send a warning saying you sent me data but you are nt my data
          printf("Received ack from a stranger\n");
          char msg[] = "You sent Ack but you are not my neighbor";
          create_warning(&response,strlen(msg),msg);
          dgcp_send(s,ipv6,port,response);
          */
          // else
          // remove the sender from the list of data, if he even exists
     }
     else
     {
          char msg[] = "Your ack tlv has a bad size";
          uint8_t code = 3;
          create_goaway(&response,strlen(msg)+1,code,msg);
          dgcp_send(s,ipv6,port,response);
     }
}
void goaway_handler(int s, unsigned char ipv6[], int port, msg_body tlv)
{
     dgc_packet response = {0};

     hexdump((char *)&tlv,tlv.goaway.length+2,"Received");

     /*
     if the neighbor is not in my neighbor list send a warning saying you sent me data but you are nt my data
     printf("Received goaway from a stranger\n");
     char msg[] = "You went away but I don't know you";
     create_warning(&response,strlen(msg),msg);
     dgcp_send(s,ipv6,port,response);
     */
     if ( tlv.goaway.code == 1 )
     {
          printf("%s is leaving the network\n",ipv6);
          // move ipv6 from neighbot list to potential
     }
     else if ( tlv.goaway.code == 2 )
     {
          printf("%s is saying that we didn't ack his data or we didn't send him a hello since long time.\n",ipv6);
          // move ipv6 from neighbot list to potential
          // get the id of the sender
          // create_long_hello(&response,his_id);
          // dgcp_send(s,ipv6,port,response);
     }
     else if ( tlv.goaway.code == 3 )
     {
          printf("%s is saying that we didn't respect the protocol\n", ipv6);
          // move ipv6 from neighbor list to  potential
     }
     else
     {
          printf("%s is asking us to remove him for an unknown reason\n",ipv6 );
          // move ipv6 from neighbor list to potential
     }
}
