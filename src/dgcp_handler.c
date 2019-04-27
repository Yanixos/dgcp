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
#include "neighborsController.h"

uint64_t his_id;


void dgcp_send(int s, unsigned char ipv6[], uint16_t port, dgc_packet p2send)
{
     struct sockaddr_in6 his_peer6 = {0};

     his_peer6.sin6_family = AF_INET6;
     his_peer6.sin6_port = port;
     memcpy((unsigned char*) &his_peer6.sin6_addr,ipv6,16);

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

     fd_set read_fds;
     fd_set write_fds;
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
               time_t t = time(NULL);
               getpeername(s, (struct sockaddr *) &peer6, &addrlen);
               char str[INET6_ADDRSTRLEN];
               if(inet_ntop(AF_INET6, &peer6.sin6_addr, str, sizeof(str)))
                     printf("connection from %s at %d\n", str, ntohs(peer6.sin6_port));
               //hexdump(&p2recv, p2recv.header.body_length+4, "Received");
               header_handler(s,&peer6.sin6_add,peer6.sin6_port,p2recv,t)
               FD_ZERO(&write_fds);
			FD_SET(s, &write_fds);
			if(FD_ISSET(s, &write_fds))
               	FD_CLR(s, &write_fds);


          }
     }
     pthread_mutex_unlock(&lock);
     pthread_exit(NULL);
     return NULL;
}

void header_handler(int s, unsigned char ipv6[], uint16_t port, dgc_packet p2recv, time_t t)
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
          call_tlv_handler(s,ipv6,port,tlv,type,t);
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


void call_tlv_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, uint8_t type, time_t t)
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
               hello_handler(s,ipv6,port,tlv,t);
               break;
          case 3 :
               neighbor_handler(s,ipv6,port,tlv,t);
               break;
          case 4 :
               data_handler(s,ipv6,port,tlv,t);
               break;
          case 5 :
               ack_handler(s,ipv6,port,tlv,t);
               break;
          case 6 :
               goaway_handler(s,ipv6,port,tlv,t);
               break;
          case 7 :
               printf("%s sent a warning : %s\n",ipv6,tlv.warning.message);
               break;
     }
}

void hello_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t)
{
     dgc_packet response = {0};
     recent_neighbors *current;
     ip_port* key;
     key->ip = ipv6;
     key->port = port;
     if ( tlv.s_hello.length == 8 )
     {
          hexdump((char *)&tlv,tlv.s_hello.length+2,"Received");
          if (  (current = search_recent_neighbors_key(MY_RN,key)) )
               current->hello_t = t;
          else
               create_recent_neighbor(0,key,0,t,0);
          delete_potential_neighbor(key);
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
               if (search_id_RN(MY_RN,tlv.l_hello.src_id,&current,&precedent))
               {
                    current->hello_t = t;
                    current->long_hello_t = t;
               }
               else
                    create_recent_neighbor(tlv.l_hello.src_id,key,1,t,t);
               delete_potential_neighbor(key);
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

void neighbor_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t)
{
     dgc_packet response = {0};
     recent_neighbors *current;
     ip_port* key;
     key->ip = ipv6;
     key->port = port;

     if ( tlv.neighbor.length == 18 )
     {
          hexdump((char *)&tlv, tlv.neighbor.length+2, "Received");
          if ( (current = search_recent_neighbors_key(MY_RN,key)) && current->symetric == 1)
               create_potentiel_neighbor(0,key);
     }
     else
     {
          printf("Received corrupted neighbor tlv\n");
          char msg[] = "Your neighbor tlv has a bad size";
          create_warning(&response,strlen(msg),msg);
          dgcp_send(s,ipv6,port,response);
     }
}

void data_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t)
{
     dgc_packet response = {0};
     recent_neighbors *current;
     ip_port* key;
     key->ip = ipv6;
     key->port = port;

     hexdump((char *)&tlv,tlv.data.length+2,"Received");
     if ( (current = search_recent_neighbors_key(MY_RN,key)) == NULL || current->symetric == 0)
     {
          printf("Received ack from a stranger\n");
          char msg[] = "You sent Data but you are not symetric";
          create_warning(&response,strlen(msg),msg);
          dgcp_send(s,ipv6,port,response);
          retun;
     }
     int i;
     data_key dkey = {tlv.data.id,tlv.data.nonce};
     if ( ( i = search_data(dkey) ) == -1)
     {
          create_ack(&response,tlv.data.sender_id,tlv.data.nonce);
          dgcp_send(s,ipv6,port,response);
          if ( tlv.data.data_type == 0 )
               printf("%s\n",tlv.data.message);

          recent_neighbors* n = symetric_neighbors();
          int j = add_data(key,tlv.data.data,tlv.data.type,n);
          data_flood(s,j);
     }
     else
     {
          delete_key_RN(MY_DATA[i],key);
     }
}

void ack_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t)
{
     dgc_packet response = {0};
     recent_neighbors *current;
     ip_port* key;
     key->ip = ipv6;
     key->port = port;

     if ( tlv.ack.length == 12  )
     {
          hexdump((char *)&tlv,tlv.ack.length+2,"Received");
          if ( (current = search_key_RN(MY_RN,key)) == NULL || current->symetric == 0)
          {
               printf("Received ack from a stranger\n");
               char msg[] = "You sent Data but you are not my  neighbor";
               create_warning(&response,strlen(msg),msg);
               dgcp_send(s,ipv6,port,response);
          }
          data_key dkey = {tlv.data.id,tlv.data.nonce};
          int i;
          if ( ( i = search_data(dkey) ) == -1)
          {
               char msg[] = "You sent Ack for Data that i didn't send";
               create_warning(&response,strlen(msg),msg);
               dgcp_send(s,ipv6,port,response);
          }
          else
               delete_key_RN(MY_DATA[i],key);
     }
     else
     {
          char msg[] = "Your ack tlv has a bad size";
          uint8_t code = 3;
          create_goaway(&response,strlen(msg)+1,code,msg);
          dgcp_send(s,ipv6,port,response);
     }
}
void goaway_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t)
{
     dgc_packet response = {0};
     recent_neighbors *current;
     ip_port* key;
     key->ip = ipv6;
     key->port = port;

     hexdump((char *)&tlv,tlv.goaway.length+2,"Received");

     if ( (current = search_recent_neighbors_key(MY_RN,key)) == NULL )
     {
          printf("Received goaway from a stranger\n");
          char msg[] = "You went away but I don't know you";
          create_warning(&response,strlen(msg),msg);
          dgcp_send(s,ipv6,port,response);
          return;
     }

     if ( tlv.goaway.code == 1 )
          printf("%s is leaving the network\n",ipv6);

     else if ( tlv.goaway.code == 2 )
          printf("%s is saying that we didn't ack his data or we didn't send him a hello since long time.\n",ipv6);
     else if ( tlv.goaway.code == 3 )
          printf("%s is saying that we didn't respect the protocol\n", ipv6);
     else
          printf("%s is asking us to remove him for an unknown reason\n",ipv6 );

     if ( current->symetric )
          current->symetric = 0;
     else
          move_to_potential(current->key, current->id,0);
}

void data_flood(int s, int i)
{
     int j = 0;
     while ( j  < RETRIES )
     {
          recent_neighbors* tmp1 = DATA[i].data_neighbors;
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
                    add_data(&p2send,strlen(DATA[i].data),DATA[i].key.id,DATA[i].key.nonce,DATA[i].type,DATA[i].data);
                    dgcp_send(s,tmp2->ip,tmp2->port,p2send);
                    tmp2 = tmp2->next;
               }
               tmp1 = tmp1->next;
          }
          j++;
     }
     flood_clean(s,i);
}

void flood_clean(int s,int i)
{
     recent_neighbors* tmp1 = DATA[i].data_neighbors;
     while ( tmp1 != NULL )
     {
          ip_port* tmp2 = tmp1->key;
          while ( tmp2 != NULL )
          {
               dgc_packet p2send = {0};
               char msg[50];
               sprintf(msg,"You didn't ack this data : %d:%d",DATA[i].key.id,DATA[i].key.nonce);
               create_goaway(&p2send,strlen(msg),2,msg);
               dgcp_send(s,tmp2->ip,tmp2->port,p2send);
               tmp2 = tmp2->next;
          }
          tmp1 = tmp1->next;
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

void check_neighbors(int s)
{
     recent_neighbors* tmp1 = MY_RN;
     time_t s = time(NULL);
     while ( tmp1 != NULL )
     {
          if ( tmp1->symetric )
          {
               if ( tmp1->long_hello_t > s + 120 )
               {
                    if ( tmp1->hello_t > s + 120 )
                         move_to_potential(tmp1->key,tmp1->id,1);
                    else
                         tmp1->symetric = 0;
               }
          }
          else
          {
               if ( tmp1->hello_t > s + 120 )
                    move_to_potential(tmp1->key,tmp1->id,1);
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

void move_to_potential(ip_port* key, uint64_t id,int flag)
{
     if ( !flag )
     {
          create_potentiel_neighbor(id,key);
          delete_key_RN(MY_RN,key);
          return ;
     }
     for (ip_port* tmp=key; tmp != null; tmp = tmp->next)
     {

          dgc_packet p2send = {0};
          char msg[] = "You didn't say hello since 2 minutes ago";
          create_goaway(&p2send,strlen(msg),2,msg);
          dgcp_send(s,tmp->ip,tmp->port,p2send);
          delete_key_RN(MY_RN,tmp);
     }
}
