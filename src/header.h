#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define MSG_SIZE 4096-8    // UDP Header is 8 bytes

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
} msg_hd;

typedef struct
{
     type_t type;             // if (type != [0-7]) -> ignore
     uint8_t length;          // if (length > body_length-2) -> ignore out of bounds
     union
     {
          struct
          {
               char trash[4082];
          } pad1;

          struct
          {
               char MBZ[4082];
          } padn;

          struct
          {
               uint64_t src_id;              // the hash of the mac-address
          } hello_short ;

          struct
          {
               uint64_t src_id;
               uint64_t dest_id;
          } hello_long;

          struct
          {
               unsigned char ip[16];
               uint16_t port;
          } neighbour;

          struct
          {
               uint64_t sender_id;
               unit32_t nonce;          // encode actual time
               unit8_t type;            // (type == 0) ? print : forward
               char message[4069];
          } data;

          struct
          {
               uint64_t sender_id;
               unit32_t nonce;
          } ack;

          struct
          {
               uint8_t code;                // [0-3]
               char messgae[4081];
          } goaway;

          struct
          {
               char message[4082];
          } warning;

    } value;
} msg_body;

typedef struct
{
     msg_hd header;
     msg_body tlv;
} msg_packet;


typedef struct
{
     unsigned char ip[16];
     uint16_t port;
} peer;

typedef struct neighbour
{
     uint64_t id;
     peer*  ip_port;
     time_t hello_t;
     time_t long_hello_t;
     struct neighbour* next;
} neighbour;

typedef struct
{
     neighbour* symteric;
     neighbour* potential;
} neighours_list;



uint64_t init_peer();
int check_header(msg_hd header);
int msg_type(char* buffer);
