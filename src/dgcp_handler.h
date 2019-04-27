#ifndef DGCP_HANDLER_H_INCLUDED
#define DGCP_HANDLER_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include "header.h"

pthread_t tid[2];
pthread_mutex_t lock;

extern void dgcp_send(int s, unsigned char ipv6[], uint16_t port, dgc_packet p2send);
extern void *dgcp_recv(void *arguments);

extern void header_handler(int s, unsigned char ipv6[], uint16_t port, dgc_packet p2recv, time_t t);
extern void call_tlv_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, uint8_t type,time_t t);
extern void hello_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t);
extern void neighbor_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t);
extern void data_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t);
extern void ack_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t);
extern void goaway_handler(int s, unsigned char ipv6[], uint16_t port, msg_body tlv, time_t t);
extern void data_flood(int s, int i);
extern void flood_clean(int s,int i);
extern void share_neighbors(int s);
extern void check_neighbors(int s);
extern void move_to_potential(ip_port* key, uint64_t id,int flag);

#endif
