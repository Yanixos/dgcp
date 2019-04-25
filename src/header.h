#ifndef HEADER_H_INCLUDED
#define HEDAER_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define DGCP_SIZE 4096-8    // UDP Header is 8 bytes
uint64_t MY_ID;

typedef uint8_t type_t;
enum type_enum
{
    PAD1                 = 0x00,
    PADN                 = 0x01,
    HELLO_SHORT          = 0x02,
    HELLO_LONG           = 0x02,
    NEIGHBOR             = 0x03,
    DATA                 = 0x04,
    ACK                  = 0x05,
    GOAWAY               = 0x06,
    WARNING              = 0x07,
};


typedef struct
{
     uint8_t magic;           // (magic != 93)  -> ignore
     uint8_t version;         // (version != 2) -> ignore
     uint16_t body_length;    // (body_length > PACKET_SIZE-4) -> ignore out of bounds data
} __attribute__((packed, scalar_storage_order("big-endian"))) msg_hd;


typedef struct
{
     type_t type;             // if (type != [0-7]) -> ignore
} __attribute__((packed, scalar_storage_order("big-endian"))) tlv0;

typedef struct
{
     type_t type;             // if (type != [0-7]) -> ignore
     uint8_t length;          // if (length > body_length-2) -> ignore out of bounds
     char MBZ[DGCP_SIZE-6];
} __attribute__((packed, scalar_storage_order("big-endian"))) tlv1;

typedef struct
{
     type_t type;             // if (type != [0-7]) -> ignore
     uint8_t length;          // if (length > body_length-2) -> ignore out of bounds
     uint64_t src_id;                                            // the hash of the mac-address

} __attribute__((packed, scalar_storage_order("big-endian"))) tlv21;

typedef struct
{
     type_t type;             // if (type != [0-7]) -> ignore
     uint8_t length;          // if (length > body_length-2) -> ignore out of bounds
     uint64_t src_id;
     uint64_t dst_id;

} __attribute__((packed, scalar_storage_order("big-endian"))) tlv22;

typedef struct
{
     type_t type;             // if (type != [0-7]) -> ignore
     uint8_t length;          // if (length > body_length-2) -> ignore out of bounds
     unsigned char ip[16];
     uint16_t port;
} __attribute__((packed, scalar_storage_order("big-endian"))) tlv3;

typedef struct
{
     type_t type;             // if (type != [0-7]) -> ignore
     uint8_t length;          // if (length > body_length-2) -> ignore out of bounds
     uint64_t sender_id;
     uint32_t nonce;          // encode actual time
     uint8_t data_type;            // (type == 0) ? print : forward
     char message[DGCP_SIZE-6-13];
} __attribute__((packed, scalar_storage_order("big-endian"))) tlv4;

typedef struct
{
     type_t type;             // if (type != [0-7]) -> ignore
     uint8_t length;          // if (length > body_length-2) -> ignore out of bounds
     uint64_t sender_id;
     uint32_t nonce;
} __attribute__((packed, scalar_storage_order("big-endian"))) tlv5;

typedef struct
{
     type_t type;             // if (type != [0-7]) -> ignore
     uint8_t length;          // if (length > body_length-2) -> ignore out of bounds
     uint8_t code;                           // [0-3]
     char message[DGCP_SIZE-6-1];
} __attribute__((packed, scalar_storage_order("big-endian"))) tlv6;

typedef struct
{
     type_t type;             // if (type != [0-7]) -> ignore
     uint8_t length;          // if (length > body_length-2) -> ignore out of bounds
     char message[DGCP_SIZE-6];
} __attribute__((packed, scalar_storage_order("big-endian"))) tlv7;

typedef union _msg_body
{
     tlv0  pad1;
     tlv1  padn;
     tlv21 s_hello;
     tlv22 l_hello;
     tlv3  neighbor;
     tlv4  data;
     tlv5  ack;
     tlv6  goaway;
     tlv7  warning;
} msg_body;

typedef struct
{
     msg_hd header;
     msg_body tlv;
} dgc_packet;


typedef struct
{
     unsigned char ip[16];
     uint16_t port;
}  __attribute__((packed, scalar_storage_order("big-endian"))) ip_port;

typedef struct recent_neighbors
{
     uint64_t id;
     ip_port*  ip_port;
     int symetric
     time_t hello_t;
     time_t long_hello_t;
     struct recent_neighbors* next;
}  __attribute__((packed, scalar_storage_order("big-endian"))) recent_neighbors;

typedef struct potential_neighbors
{
     uint64_t id;
     ip_port*  ip_port;
     struct potential_neighbors* next;
}  __attribute__((packed, scalar_storage_order("big-endian"))) potential_neighbors;

typedef struct
{
     uint64_t id;
     uint32_t nonce;
}  __attribute__((packed, scalar_storage_order("big-endian"))) data_key;

typedef struct data_list
{
     data_key key;
     time_t data_time;
     char* data;
     potential_neighbors* data_neighbors;
     uint8_t nb;
} __attribute__((packed, scalar_storage_order("big-endian"))) data_list;

extern uint64_t generate_id();
extern uint32_t generate_nonce();
extern int check_header(dgc_packet* m);

extern void create_header(dgc_packet* m, uint16_t length);
extern void create_pad1(dgc_packet* m);
extern void create_padN(dgc_packet* m, uint8_t length);
extern void create_short_hello(dgc_packet* m);
extern void create_long_hello(dgc_packet* m, uint64_t dst_id);
extern void create_neighbor(dgc_packet* m, unsigned char ipv6[], int port);
extern void create_data(dgc_packet* m, uint8_t length, uint64_t sender_id, uint32_t nonce, uint8_t type, char* msg);
extern void create_ack(dgc_packet* m, uint64_t sender_id, uint32_t nonce);
extern void create_goaway(dgc_packet* m, uint8_t length, uint8_t code, char* msg);
extern void create_warning(dgc_packet* m, uint8_t length, char* msg);


#endif
