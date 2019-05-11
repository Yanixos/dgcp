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
#include <math.h>
#include "hexdump.h"
#include "dgcp_handler.h"

int rounds = 0;                                                        // counter for each 30sec round
int debug = 1;                                                         // verbose mode

int get_peer_info(char* hostname, char* port, int *sock, struct sockaddr_in6 *addr)
{
     struct addrinfo hints, *r, *p;

     memset(&hints, 0, sizeof(hints));

     hints.ai_family = AF_INET6;
     hints.ai_socktype = SOCK_DGRAM;
     hints.ai_flags = (AI_V4MAPPED | AI_ALL);

     if ((getaddrinfo(hostname, port, &hints, &r)) != 0 || NULL == r)
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

void *routine(void *args)                                              // each 30sec send long hello, each 2mn share neighbors, each 3mn discover neighbors
{
     int *arg = (int*) args;
     int s = *arg;
     recent_neighbors* tmp1;
     ip_port* tmp2;

     discover_neighbors(s);                                           // discover neighbors for the first time

     while (1)
     {
          tmp1 = MY_RN;                                               // get the actual recent neighbor list
          while  ( tmp1 != NULL )
          {

               if ( debug )
               {
                    print_recent(MY_RN);
                    print_potential(MY_PN);
               }
               sleep(30);                                             // wait for 30 sec
               rounds++;

               tmp2 = tmp1->key;
               while ( tmp2 )
               {
                    dgc_packet p2send = {0};
                    create_long_hello(&p2send,tmp1->id);
                    pthread_mutex_lock(&lock);                        // lock the thread for synchronizing the R/W of the shared data
                    dgcp_send(s,tmp2->ip,tmp2->port,p2send);          // send a long hello for the recent neighbors
                    pthread_mutex_unlock(&lock);                      // unlock the thread for synchronizing the R/W of the shared data
                    tmp2 = tmp2->next;
               }

               if ( rounds % 4 == 0 )                                 // if 2mn passed
               {
                    share_neighbors(s);                               // share my neighbors
                    check_neighbors(s);                               // update neighbors data
               }

               if ( rounds % 6 == 0 )                                 // if 3mn passed
                    discover_neighbors(s);                            // discover neighbors

               tmp1 = tmp1->next;
          }
     }
     pthread_exit(NULL);
     return NULL;
}

void *dgcp_recv(void *arguments)
{
     int *arg = (int*) arguments;
     int s = *arg;                                                                         // get the fd of the socket
     struct sockaddr_in6 peer6 = {0};                                                     // initialize our struct
     socklen_t addrlen = sizeof(peer6);
     dgc_packet p2recv = {0};

     fd_set read_fds;
     fd_set write_fds;
     int rc = fcntl(s, F_GETFL);                                                          // get the file access mode of the socket
     if ( rc < 0 )
     {
          perror("fcntl F_GETFL ");
          exit(EXIT_FAILURE);
     }
     rc = fcntl(s, F_SETFL, rc | O_NONBLOCK);          // set the file access mode to non blocking for getting multiple connections with non block mode
     if ( rc < 0 )
     {
          perror("fcntl F_SETFL ");
          exit(EXIT_FAILURE);
     }

     while (1) // read for ever
     {
          FD_ZERO(&read_fds);
          FD_SET(s, &read_fds);                                                           // we set the file descriptor for reading
          int n = select(s+1, &read_fds, 0, 0, 0);                                        // handle multiple connections
          if(n < 0)
          {
               perror("select ");
               close(s);
               exit(EXIT_FAILURE);
          }
          if(FD_ISSET(s, &read_fds))                                            // once the file descriptor is ready for reading
          {
               pthread_mutex_lock(&lock);                                                               // lock the thread for synchronizing the R/W of the shared data
               if ( recvfrom(s, (char*) &p2recv, DGCP_SIZE , 0,  (struct sockaddr *)  &peer6, &addrlen) < 0 ) // we read data
               {
                    perror("recvfrom ");
                    close(s);
                    exit(EXIT_FAILURE);
               }
               time_t t = time(NULL);                                                          // keep track of the time of receiving data
               FD_CLR(s, &read_fds);                                                           // close the read fd
               if ( debug )
               {
                    getpeername(s, (struct sockaddr *) &peer6, &addrlen);
                    char str[INET6_ADDRSTRLEN];
                    if(inet_ntop(AF_INET6, &peer6.sin6_addr, str, sizeof(str)))
                          printf("connection from %s at %d\n", str, ntohs(peer6.sin6_port));
               }
               FD_ZERO(&write_fds);
			FD_SET(s, &write_fds);                                                            // prepare the fd for writing data
			if(FD_ISSET(s, &write_fds))                                                       // once the fd is ready
                    header_handler(s,(unsigned char*)&peer6.sin6_addr,peer6.sin6_port,p2recv,t); // handle the packet
               FD_CLR(s, &write_fds);                                                            // close the fd for writing
               pthread_mutex_unlock(&lock);                         // unlock the thread for synchronizing the R/W of the shared data
          }
     }
     pthread_exit(NULL);
     return NULL;
}

void *data_flood(void *args)
{
     int *arg = (int*) args;
     int s = *arg;
     int i;
     while ( 1 )
     {
          if ( POS < COUNT )
          {
               int j = 0;
               while ( j  < RETRIES )                  // flood data RETRIES times
               {
                    i = POS % DATA_SIZE;
                    recent_neighbors* tmp1 = MY_DATA[i].data_neighbors;    // get the neighbor list
                    while ( tmp1 != NULL )
                    {
                         ip_port* tmp2 = tmp1->key;
                         while ( tmp2 != NULL )                  // for each neighbor, go through all his (ip,port)
                         {
                              double t1 = pow(2,j-1);
                              double t2 = pow(2,j);
                              int t = (t1 + t2) / 2;
                              sleep(t);                          // give that neighbor time to ack the data sent
                              dgc_packet p2send = {0};
                              pthread_mutex_lock(&lock);                                        // lock the thread for synchronizing the R/W of the shared data
                              create_data(&p2send,strlen(MY_DATA[i].data),MY_DATA[i].key.id,MY_DATA[i].key.nonce,MY_DATA[i].type,MY_DATA[i].data);
                              pthread_mutex_unlock(&lock);                                        // unlock the thread for synchronizing the R/W of the shared data
                              dgcp_send(s,tmp2->ip,tmp2->port,p2send);          // send the data
                              tmp2 = tmp2->next;
                         }
                         tmp1 = tmp1->next;
                    }
                    j++;
               }
               flood_clean(s,i);                  // delete those who didn't respond for the data flood
               POS++;
          }
     }
     pthread_exit(NULL);
     return NULL;
}

void dgcp_send(int s, unsigned char ipv6[], uint16_t port, dgc_packet p2send)
{
     struct sockaddr_in6 his_peer6 = {0};                                                    // initialize the struct of the destination peer

     his_peer6.sin6_family = AF_INET6;
     his_peer6.sin6_port = port;
     memcpy((unsigned char*) &his_peer6.sin6_addr,ipv6,16);

     socklen_t addrlen = sizeof(his_peer6);
     int size = p2send.header.body_length + 4;                                               // get the size of the packet to send
     if ( sendto(s, (char*) &p2send, size, 0, (struct sockaddr *) &his_peer6, addrlen) < 0 ) // send the packet
     {
          char str[INET6_ADDRSTRLEN];
          if(inet_ntop(AF_INET6, &ipv6, str, sizeof(str)))
                printf("Unreachable host : %s at %d\n", str, ntohs(port));
          return ;
     }
     if ( debug )
          hexdump((char*) &p2send.tlv, p2send.tlv.padn.length+2,"Sent");                            // print the sent packet
}

void header_handler(int s, unsigned char ipv6[], uint16_t port, dgc_packet p2recv, time_t t)
{
     ip_port* key = calloc(1,sizeof(ip_port));                   // save the (ip,port) of the sender
     memcpy(key->ip,ipv6,16);
     key->port = port;

     unsigned char buff[DGCP_SIZE];
     memcpy(buff, (const unsigned char*) &p2recv, sizeof(p2recv));         // cast the dgc_packet to unsigned char array
     uint16_t body_length = p2recv.header.body_length;

     dgc_packet response = {0};

     if ( ! check_header(&p2recv) )                                        // if the header is not correct
     {
          printf("Non dgcp packet\n");
          char msg[] = "Your are not respecting the protocol (check your header)";
          uint8_t code = 3;
          create_goaway(&response,strlen(msg)+1,code,msg);
          dgcp_send(s,ipv6,port,response);                              // send a goaway of code 3 saying that the sender didn't respect the protocol
          recent_neighbors* n = search_recent_neighbors_key(MY_RN,key); // if the sender exists in our neighbor list, move him to potential list
          if ( n != NULL)
               move_to_potential(s,key,n->id,0);
          return ;
     }

     memmove(buff, buff+4, DGCP_SIZE-4);                              // we move to the body
     uint8_t type = buff[0];


     if ( type < 0 || type > 7 )                                      // if the type is not between 0 and 7
     {
          printf("Unknown tlv type\n");
          char msg[] = "Your are not respecting the protocol (check your tlv type)";
          uint8_t code = 3;
          create_goaway(&response,strlen(msg)+1,code,msg);
          dgcp_send(s,ipv6,port,response);                       // send a goaway of code 3 saying that the sender didn't respect the protocol
          recent_neighbors* n = search_recent_neighbors_key(MY_RN,key); // if the sender exists in our neighbor list, move him to potential list
          if ( n != NULL)
               move_to_potential(s,key,n->id,0);
          return ;
     }

     if ( body_length > DGCP_SIZE )                              // if the body length is bigger than the PMTU
     {
          printf("Packet size is larger than the PMTU\n");
          char msg[] = "Your packet is too large, extradata will be ignored";
          create_warning(&response,strlen(msg),msg);
          dgcp_send(s,ipv6,port,response);                       // send a warning that says that the packet was bigger and the extra data will be ignored
     }

     uint8_t tlv_length = ( buff[1] > 0 ? buff[1] :  -1 ) ;
     msg_body tlv = {0};
     unsigned char* tmp = buff;

     do                                                          // parse the TLVs
     {
          memset(&tlv,0x00,sizeof(msg_body));
          memcpy(&tlv, (msg_body*) tmp,tlv_length+2);            // cast the tlv from unsigned char to msg_body
          type = tmp[0];
          call_tlv_handler(s,ipv6,port,tlv,type,t);              // handle the tlv
          if ( type == 0 )                                       // update the variable body length
          {
               body_length -= 1;
               tmp += 1;
          }
          else
          {
               body_length -= ( tlv_length + 2);                 // update the variable body length
               tmp += tlv_length + 2;
          }
          tlv_length = tmp[1];                                   // go to the next tlv
     } while ( body_length > tlv_length + 1 );
}


void call_tlv_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, uint8_t type, time_t t)
{
     switch ( type )
     {
          case 0 :
               if ( debug )
                    hexdump((char *)&tlv,1,"Received");                    // if pad1 received then just print it for the log, not actions
               break;
          case 1 :
               if ( debug )
                    hexdump((char *)&tlv,tlv.padn.length+2,"Received");    // if padn received then just print it for the log, not actions
               break;
          case 2 :
               hello_handler(s,ipv6,port,tlv,t);                      // call hello handler
               break;
          case 3 :
               neighbor_handler(s,ipv6,port,tlv,t);                   // call neighbor_handler
               break;
          case 4 :
               data_handler(s,ipv6,port,tlv,t);                       // call data_handler
               break;
          case 5 :
               ack_handler(s,ipv6,port,tlv,t);                        // call ack_handler
               break;
          case 6 :
               goaway_handler(s,ipv6,port,tlv,t);                     // call goaway_handler
               break;
          case 7 :
               if ( debug )
                    printf("he sent a warning : %s\n",tlv.warning.message);// if a warning is received then print it for the lo, no actions
               break;
     }
}

void hello_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t)
{
     dgc_packet response = {0};
     recent_neighbors *current, *precedent;
     ip_port* key = calloc(1,sizeof(ip_port));                   // save the (ip,port) of the sender
     memcpy(key->ip,ipv6,16);
     key->port = port;
     if ( tlv.s_hello.length == 8 )                              // if it's a short hello
     {
          if ( debug )
               hexdump((char *)&tlv,tlv.s_hello.length+2,"Received");
          if (  (current = search_recent_neighbors_key(MY_RN,key)) != NULL )    // if the sender exists in my recent_neighbors list
               current->hello_t = t;                             // refresh the time of hello
          else
               create_recent_neighbor(0,key,0,t,0);              // else add him to my recent neighbors with his information
          delete_key_PN(&MY_PN,key);                             // if he was in the potential neighbors we delete him
     }
     else if ( tlv.s_hello.length == 16 )                        // if it's a long hello
     {
          if ( debug )
               hexdump((char *)&tlv,tlv.l_hello.length+2,"Received");
          if ( tlv.l_hello.dst_id != MY_ID )                    // if the destination id is not mine
          {
               printf("The destination id is wrong\n");
               char msg[] = "Misdirection hello (that's not my id)";
               create_warning(&response,strlen(msg),msg);
               dgcp_send(s,ipv6,port,response);                // send a warning saying that the id is not mine
          }
          else
          {
               if (search_id_RN(MY_RN,tlv.l_hello.src_id,&current,&precedent))  // if the sender exists in my recent neighbor list
               {
                    current->symetric = 1;                                      // update his information
                    current->hello_t = t;
                    current->long_hello_t = t;
               }
               else
                    create_recent_neighbor(tlv.l_hello.src_id,key,1,t,t);       // else add him to my recent neighbor list
               delete_key_PN(&MY_PN,key);                                       // if he was in the potential neighbor list, we delete him
          }
     }
     else                                              // if the length of the hello tlv is wrong
     {
          printf("Received corrupted hello tlv\n");
          char msg[] = "Your are not respecting the protocol (check your hello tlv length)";
          uint8_t code = 3;
          create_goaway(&response,strlen(msg)+1,code,msg);
          dgcp_send(s,ipv6,port,response);                  // we send a goaway
          recent_neighbors* n = search_recent_neighbors_key(MY_RN,key); // if the sender exists in our neighbor list, move him to potential list
          if ( n != NULL)
               move_to_potential(s,key,n->id,0);
     }
}

void neighbor_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t)
{
     dgc_packet response = {0};
     recent_neighbors *current;
     ip_port* key = calloc(1,sizeof(ip_port));
     memcpy(key->ip,ipv6,16);
     key->port = port;

     if ( tlv.neighbor.length == 18 )             // if the tlv length is correct
     {
          if ( debug )
               hexdump((char *)&tlv, tlv.neighbor.length+2, "Received");
          if ( (current = search_recent_neighbors_key(MY_RN,key)) && current->symetric == 1)   // if the sender is our symetric neighbor
          {
               ip_port* nn = calloc(1,sizeof(ip_port));
               memcpy(nn->ip,tlv.neighbor.ip,16);
               nn->port = tlv.neighbor.port;
               create_potentiel_neighbor(0,nn);             // add his neighbor to our potential neighbor list
          }
     }
     else
     {
          printf("Received corrupted neighbor tlv\n");
          char msg[] = "Your are not respecting the protocol (check your neighbor tlv length)";
          uint8_t code = 3;
          create_goaway(&response,strlen(msg)+1,code,msg);
          dgcp_send(s,ipv6,port,response);                  // we send a goaway
          recent_neighbors* n = search_recent_neighbors_key(MY_RN,key); // if the sender exists in our neighbor list, move him to potential list
          if ( n != NULL)
               move_to_potential(s,key,n->id,0);
     }
}

void data_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t)
{
     dgc_packet response = {0};
     recent_neighbors *current;
     ip_port* key = calloc(1,sizeof(ip_port));         // save the (ip,port) of the sender
     memcpy(key->ip,ipv6,16);
     key->port = port;

     if ( debug )
          hexdump((char *)&tlv,tlv.data.length+2,"Received");

     if ( (current = search_recent_neighbors_key(MY_RN,key)) == NULL || current->symetric == 0 ) // if the sender is not a syemtric neighbor
     {
          printf("Received data from a stranger\n");
          char msg[] = "You sent Data but you are not symetric";
          create_warning(&response,strlen(msg),msg);
          dgcp_send(s,ipv6,port,response);                  // send him a warning that says that he needs to be our neighbor before he sends us data
          return;
     }
     int i;
     data_key dkey = {tlv.data.sender_id,tlv.data.nonce};       // get the data key
     if ( ( i = search_data(dkey) ) == -1)                       // if the data doesn't exists in our data array
     {
          create_ack(&response,tlv.data.sender_id,tlv.data.nonce);
          dgcp_send(s,ipv6,port,response);                        // send back an ack
          if ( tlv.data.data_type == 0 )                          // print the data if its type of 0
               printf("%s\n",tlv.data.message);

          recent_neighbors* n = symetric_neighbors();            // get the actual syemtric neighbors
          add_data(dkey,tlv.data.message,tlv.data.type,n);    // add the data to the data array
     }
     else                                                             // if the data already exists in the data array
     {
          recent_neighbors* tmp = MY_DATA[i].data_neighbors;         // get the neighbor list of that data
          delete_key_RN(&tmp,key);                                    // remove the sender from that list
     }
}

void ack_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t)
{
     dgc_packet response = {0};
     recent_neighbors *current;
     ip_port* key = calloc(1,sizeof(ip_port));              // save the (ip,port) of the sender
     memcpy(key->ip,ipv6,16);
     key->port = port;
     if ( tlv.ack.length == 12  )                           // if the ack length is correct
     {
          if ( debug )
               hexdump((char *)&tlv,tlv.ack.length+2,"Received");
          if ( (current = search_recent_neighbors_key(MY_RN,key)) == NULL || current->symetric == 0) // if the sender is not a syemtric neighbor
          {
               printf("Received ack from a stranger\n");
               char msg[] = "You sent Data but you are not my  neighbor";
               create_warning(&response,strlen(msg),msg);
               dgcp_send(s,ipv6,port,response);                  // send a warning that says that a neighbor is needed before sending ack
          }
          data_key dkey = {tlv.data.sender_id,tlv.data.nonce};   // get the data key
          int i;
          if ( ( i = search_data(dkey) ) == -1)                  // if the data that has been acked doesn't exists in the data array
          {
               char msg[] = "You sent Ack for Data that i didn't send";
               create_warning(&response,strlen(msg),msg);
               dgcp_send(s,ipv6,port,response);                  // send a warning that says that the data that has been acked doesn't exist in the data array
          }
          else                                              // if that data exists
               delete_key_RN(&(MY_DATA[i].data_neighbors),key);                             // delete the sender from that list
     }
     else                                                        // if the neighbor tlv length is not correct
     {
          printf("Received corrupted ack tlv\n");
          char msg[] = "Your are not respecting the protocol (check your ack tlv length)";
          uint8_t code = 3;
          create_goaway(&response,strlen(msg)+1,code,msg);
          dgcp_send(s,ipv6,port,response);                       // send a goaway of code 3 and message that says that the ack length is wrong
          recent_neighbors* n = search_recent_neighbors_key(MY_RN,key); // if the sender exists in our neighbor list, move him to potential list
          if ( n != NULL)
               move_to_potential(s,key,n->id,0);
     }
}
void goaway_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t)
{
     dgc_packet response = {0};
     recent_neighbors *current;
     ip_port* key = calloc(1,sizeof(ip_port));
     memcpy(key->ip,ipv6,16);
     key->port = port;

     if ( debug )
          hexdump((char *)&tlv,tlv.goaway.length+2,"Received");

     if ( (current = search_recent_neighbors_key(MY_RN,key)) == NULL )     // if the sender doesn't exists in our recent neighbor list
     {
          printf("Received goaway from a stranger\n");
          char msg[] = "You went away but I don't know you";
          create_warning(&response,strlen(msg),msg);
          dgcp_send(s,ipv6,port,response);                       // send a warning that says that a neighbor relation is needed before sending a goaway tlv
          return;
     }

     if ( tlv.goaway.code == 1 )
          printf("he is leaving the network : %s\n",tlv.goaway.message);

     else if ( tlv.goaway.code == 2 )
          printf("he is saying that we didn't ack his data or we didn't send him a hello since long time.\n%s\n",tlv.goaway.message);
     else if ( tlv.goaway.code == 3 )
          printf("he is saying that we didn't respect the protocol\n%s\n", tlv.goaway.message);
     else
          printf("he is asking us to remove him for an unknown reason\n%s\n",tlv.goaway.message);

     if ( current->symetric )                     // if the sender is symetric, then make him not
          current->symetric = 0;
     else
          move_to_potential(s,current->key, current->id,0); // if the sender is not syemtric, move him to potential list
}

void flood_clean(int s,int i)
{
     recent_neighbors* tmp1 = MY_DATA[i].data_neighbors;    // get the neighbots of the data
     while ( tmp1 != NULL )
     {
          ip_port* tmp2 = tmp1->key;
          while ( tmp2 != NULL )             // for each remaining neighbor in that list
          {
               dgc_packet p2send = {0};
               char msg[50];
               sprintf(msg,"You didn't ack this data : %ld:%d",MY_DATA[i].key.id,MY_DATA[i].key.nonce);
               create_goaway(&p2send,strlen(msg),2,msg);
               dgcp_send(s,tmp2->ip,tmp2->port,p2send);               // send a goaway tlv for not ACKing that data
               move_to_potential(s,tmp2,tmp1->id,0);
               tmp2 = tmp2->next;
          }
          tmp1 = tmp1->next;
     }
}

void share_neighbors(int s)        // creates a neighbor tlv that has all the symetric neighbors
{
     unsigned char dgcp_neighbor[DGCP_SIZE] = {0};
     unsigned char* ptr = dgcp_neighbor;
     ptr[0] =  93;                      // set up the header
     ptr[1] = 2;
     uint16_t body_length = 0;
     ptr += 4;
     recent_neighbors* tmp1 = symetric_neighbors();         // get the list of symetric neighbors
     ip_port* tmp2;
     while ( tmp1 != NULL && body_length <= DGCP_SIZE - 24 )  // check for the bounds
     {
          tmp2 = tmp1->key;
          while ( tmp2 != NULL && body_length <= DGCP_SIZE - 24 )     // check for the bounds
          {
               ptr[0] = 3;                                       // tlv type
               ptr[1] = 18;                                      // tlv size
               ptr += 2;
               memcpy(ptr,tmp2->ip,16);                          // set the ip
               ptr += 16;
               snprintf((char*)ptr,2,"%d",tmp2->port);           // set the port
               ptr += 2;
               body_length += 20;                                // update the body length
               tmp2 = tmp2->next;                                // move to the next neighbor
          }
          tmp1 = tmp1->next;
     }
     uint16_t be = htons(body_length);
     snprintf((char*)dgcp_neighbor+2,2,"%d",be);                 // set the body length
     dgc_packet p2send = {0};
     memcpy(&p2send, (dgc_packet*) dgcp_neighbor,body_length+4); // cast from unsigned char to dgc_packet

     tmp1 = symetric_neighbors();                                // for each symetric neighbor
     while ( tmp1 != NULL )
     {
          tmp2 = tmp1->key;
          while ( tmp2 != NULL )
          {
               dgcp_send(s,tmp2->ip,tmp2->port,p2send);          // send the list of neighbors
               tmp2 = tmp2->next;
          }
          tmp1 = tmp1->next;
     }
}

void check_neighbors(int s)        // each 2mn check for the last hello time and update the recent neighbor list
{
     recent_neighbors* tmp1 = MY_RN;
     time_t sec = time(NULL);
     while ( tmp1 != NULL )
     {
          if ( tmp1->symetric )              // if the neighbor is symetric
          {
               if ( tmp1->long_hello_t > sec + 120 )   // if the last long hello time is from more than 2mn
               {
                    if ( tmp1->hello_t > sec + 120 )   // if the short hello tie is from more than 2 mn
                         move_to_potential(s,tmp1->key,tmp1->id,1);   // move him to potential
                    else
                         tmp1->symetric = 0;           // else make him non symetric
               }
          }
          else                               // if the neighbor is not syemtric
          {
               if ( tmp1->hello_t > sec + 120 )        // is his short hello is from more than 2 n
                    move_to_potential(s,tmp1->key,tmp1->id,1);   // move him to potential neighbor
          }
          tmp1 = tmp1->next;
     }
}

void discover_neighbors(int s)
{
     if ( nb_recent_neighbors() < 8 )
     {
          potential_neighbors* tmp1 = MY_PN;
          while ( tmp1 != NULL )
          {
               ip_port* tmp2 = tmp1->key;
               while ( tmp2 != NULL)
               {
                    dgc_packet p2send = {0};
                    create_short_hello(&p2send);
                    dgcp_send(s,tmp2->ip,tmp2->port,p2send);
                    tmp2 = tmp2->next;
               }
               tmp1 = tmp1->next;
          }
     }

}

void move_to_potential(int s,ip_port* key, uint64_t id,int flag)
{
     if ( !flag )             // move only one (ip,port) of a neighbor to the potential neighbor list
     {
          create_potentiel_neighbor(id,key);      // add him to the potential
          delete_key_RN(&MY_RN,key);              // delete him from the recent
          return ;
     }
     for (ip_port* tmp=key; tmp != NULL; tmp = tmp->next) // move all the (ip,port) of that neighbor to the potential list
     {

          dgc_packet p2send = {0};
          char msg[] = "You didn't say hello recently";
          create_goaway(&p2send,strlen(msg),2,msg);
          dgcp_send(s,tmp->ip,tmp->port,p2send);
          create_potentiel_neighbor(id,tmp);      // add him to the potential
          delete_key_RN(&MY_RN,tmp);              // delete him from the recent
     }
}
