#ifndef DGCP_HANDLER_H_INCLUDED
#define DGCP_HANDLER_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include "header.h"
#include "neighborsController.h"

pthread_t tid[4];        // three threads
pthread_mutex_t lock;    // the thread lock to synchronize the shared data between threads

int rounds;
int verbose;

extern int get_peer_info(char* hostname, char* port, int *sock, struct sockaddr_in6 *addr);        // fill peer information (ip,port)

extern void *routine(void *args);                 // each 30sec send long hello, each 2mn share neighbors, each 3mn discover neighbors
extern void *dgcp_recv(void *args);               // thread for receiving data
extern void *data_flood(void *args);              // floods the data to the symetric neighbors
extern void dgcp_send(int s, unsigned char ipv6[], uint16_t port, dgc_packet p2send);     // send a dgcp packet to (upv6,port)

extern void header_handler(int s, unsigned char ipv6[], uint16_t port, dgc_packet p2recv, time_t t); // handles the received header
extern void call_tlv_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, uint8_t type,time_t t); // calls the correct tlv handler
extern void hello_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t); // handles the actions to do after receiving a hello tlv
extern void neighbor_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t); // handles the actions to do after receiving a neighbor tlv
extern void data_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t);   // handles the actions to do after receiving a data tlv
extern void ack_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t); // handles the actions to do after receiving a go away
extern void goaway_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t); // handles the actions to do after receiving a goaway tlv
extern void flood_clean(int s,int i);   // clean the after flooding actions
extern void share_neighbors(int s);     // shares the neighbor after every 2mn
extern void check_neighbors(int s);     // checks if the neighbor are still alive and refresh the list of neighbors
extern void discover_neighbors(int s);      // sends a short hello to the potential neighbors if there are few recent neighbors
extern void move_to_potential(int s, ip_port* key, uint64_t id,int flag);  // move a recent neighbor to a potential neighbor

#endif
