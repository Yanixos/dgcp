#ifndef DGCP_HANDLER_H_INCLUDED
#define DGCP_HANDLER_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include "header.h"

pthread_t tid[1];
pthread_mutex_t lock;

extern void dgcp_send(int s, unsigned char ipv6[], int port, dgc_packet p2send);
extern void *dgcp_recv(void *arguments);

extern void header_handler(int s, unsigned char ipv6[], int port, dgc_packet p2recv);
extern void call_tlv_handler(int s, unsigned char ipv6[], int port, msg_body tlv, uint8_t type);
extern void hello_handler(int s, unsigned char ipv6[], int port, msg_body tlv);
extern void neighbor_handler(int s, unsigned char ipv6[], int port, msg_body tlv);
extern void data_handler(int s, unsigned char ipv6[], int port, msg_body tlv);
extern void ack_handler(int s, unsigned char ipv6[], int port, msg_body tlv);
extern void goaway_handler(int s, unsigned char ipv6[], int port, msg_body tlv);

#endif
