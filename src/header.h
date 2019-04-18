#ifndef HEADER_H_INCLUDED
#define HEDAER_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MSG_SIZE 4096-8    // UDP Header is 8 bytes

uint64_t MY_ID;

typedef uint8_t type_t;
enum type_enum
{
    PAD1                 = 0x00,
    PADN                 = 0x01,
    HELLO_SHORT          = 0x02,
    HELLO_LONG           = 0x02,
    NEIGHBOUR            = 0x03,
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
     char MBZ[MSG_SIZE-6];
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
     char message[MSG_SIZE-6-13];
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
     char message[MSG_SIZE-6-1];
} __attribute__((packed, scalar_storage_order("big-endian"))) tlv6;

typedef struct
{
     type_t type;             // if (type != [0-7]) -> ignore
     uint8_t length;          // if (length > body_length-2) -> ignore out of bounds
     char message[MSG_SIZE-6];
} __attribute__((packed, scalar_storage_order("big-endian"))) tlv7;

union msg_body
{
     tlv0  pad1;
     tlv1  padn;
     tlv21 s_hello;
     tlv22 l_hello;
     tlv3  neighbour;
     tlv4  data;
     tlv5  ack;
     tlv6  goaway;
     tlv7  warning;
};

typedef struct
{
     msg_hd header;
     union msg_body tlv;
} msg_packet;


typedef struct
{
     unsigned char ip[16];
     uint16_t port;
}  __attribute__((packed, scalar_storage_order("big-endian"))) peer;

typedef struct neighbour_list
{
     uint64_t id;
     peer*  ip_port;
     time_t hello_t;
     time_t long_hello_t;
     int retries;
     struct neighbour_list* next;
}  __attribute__((packed, scalar_storage_order("big-endian"))) neighbour_list;

typedef struct
{
     neighbour_list* symteric;
     neighbour_list* potential;
} neighours;

typedef struct
{
     uint64_t id;
     uint32_t nonce;
}  __attribute__((packed, scalar_storage_order("big-endian"))) data_key;

typedef struct data_list
{
     data_key key;
     char* data;
     neighbour_list* dst;
} data_list;

extern uint64_t generate_id();
extern uint32_t generate_nonce();
extern int check_packet(msg_packet* m);

extern void create_header(msg_packet* m, uint16_t length);
extern void create_pad1(msg_packet* m);
extern void create_padN(msg_packet* m, uint8_t length);
extern void create_short_hello(msg_packet* m);
extern void create_long_hello(msg_packet* m, uint64_t dst_id);
extern void create_neighbour(msg_packet* m, peer* p);
extern void create_data(msg_packet* m, uint8_t length, uint64_t sender_id, uint32_t nonce, uint8_t type, char* msg);
extern void create_ack(msg_packet* m, uint32_t nonce);
extern void create_goaway(msg_packet* m, uint8_t length, uint8_t code, char* msg);
extern void create_warning(msg_packet* m, uint8_t length, char* msg);


#endif