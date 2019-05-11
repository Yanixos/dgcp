#ifndef HEADER_H_INCLUDED
#define HEADER_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define DGCP_SIZE 4096-8    // UDP Header is 8 bytes
uint64_t MY_ID;             // a pair has only one ID

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
     uint64_t src_id;         // the hash of the mac-address

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
     uint32_t nonce;               // actual time
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
     uint8_t code;                           // [0-3] if not then it's 0
     char message[DGCP_SIZE-6-1];
} __attribute__((packed, scalar_storage_order("big-endian"))) tlv6;

typedef struct
{
     type_t type;             // if (type != [0-7]) -> ignore
     uint8_t length;          // if (length > body_length-2) -> ignore out of bounds
     char message[DGCP_SIZE-6];
} __attribute__((packed, scalar_storage_order("big-endian"))) tlv7;

typedef union _msg_body       // union to make only one of the tlvs possible
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


typedef struct ip_port                  // list of ip port ( one peer can have multiple )
{
     unsigned char ip[16];
     uint16_t port;
     struct ip_port* next;
}  __attribute__((packed, scalar_storage_order("big-endian"))) ip_port;

typedef struct recent_neighbors    // the list that contains recent neighbors, symetric flag tells if a neighbor is symetric or not
{
     uint64_t id;
     ip_port*  key;
     int symetric;
     time_t hello_t;
     time_t long_hello_t;
     struct recent_neighbors* next;
}  __attribute__((packed, scalar_storage_order("big-endian"))) recent_neighbors;

typedef struct potential_neighbors      // the list of potential neighbors, the id exists because sometimes we move neighbors from recent list to here
{
     uint64_t id;
     ip_port*  key;
     struct potential_neighbors* next;
}  __attribute__((packed, scalar_storage_order("big-endian"))) potential_neighbors;

typedef struct           // the key that identifies the data
{
     uint64_t id;
     uint32_t nonce;
}  __attribute__((packed, scalar_storage_order("big-endian"))) data_key;

typedef struct data_array          // the data array structure
{
     data_key key;
     uint8_t type;
     char* data;
     recent_neighbors* data_neighbors;
} data_array;

extern uint64_t generate_id();          // generates the id from the mac address and its hash
extern uint32_t generate_nonce();       // generates the nonce of the data using the actual time
extern int check_header(dgc_packet* m); // checks if the header is correct

extern void create_header(dgc_packet* m, uint16_t length);  // creates a dgcp header
extern void create_pad1(dgc_packet* m);                     // creates a pad1 tlv
extern void create_padN(dgc_packet* m, uint8_t length);     // creates a padn tlv of a precise length
extern void create_short_hello(dgc_packet* m);              // creares a short hello tlv using MY_ID as a src_id
extern void create_long_hello(dgc_packet* m, uint64_t dst_id);   // creates a short hello tlv using MY_ID as a source and dst_id as destination
extern void create_neighbor(dgc_packet* m, unsigned char ipv6[], int port); // creates a neighbor tlv using (ipv6,port)
extern void create_data(dgc_packet* m, uint8_t length, uint64_t sender_id, uint32_t nonce, uint8_t type, char* msg); // creates a data tlv
extern void create_ack(dgc_packet* m, uint64_t sender_id, uint32_t nonce);         // creates an ack tlv
extern void create_goaway(dgc_packet* m, uint8_t length, uint8_t code, char* msg);   // creates a goaway tlv
extern void create_warning(dgc_packet* m, uint8_t length, char* msg);           // creates a warning tlv


#endif
