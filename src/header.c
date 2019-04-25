#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <time.h>
#include "header.h"

char* mac_eth0()
{
     #define MAC_SIZE 6
	struct ifreq buffer;
	int fd = socket(AF_INET6, SOCK_DGRAM, 0);
	memset(&buffer, 0x00, sizeof(buffer));
	strcpy(buffer.ifr_name, "eth0");
	ioctl(fd, SIOCGIFHWADDR, &buffer);
	close(fd);
     char* mac = malloc(MAC_SIZE*sizeof(char));
     strcpy(mac,(char*) buffer.ifr_hwaddr.sa_data);
     return mac;
}

uint64_t generate_id()
{
     uint64_t hash = 17570031337;
     int c;

     char *mac = mac_eth0();

     while ((c = *mac++))
          hash = ((hash << 5) + hash) + c;

     return hash;
}

uint32_t generate_nonce()
{
     time_t s = time(NULL);
     return (uint32_t) s;
}

int check_header(dgc_packet* m)
{
     return ( m->header.magic == 93 && m->header.version == 2 );
}

void create_header(dgc_packet* m, uint16_t length)
{
     m->header.magic = 93;
     m->header.version = 2;
     m->header.body_length = length;
}

void create_pad1(dgc_packet* m)
{
     uint16_t body_length = 1;
     create_header(m,body_length);
     m->tlv.pad1.type = 0;
}

void create_padN(dgc_packet* m, uint8_t length)
{
     uint16_t body_length = length+2;
     create_header(m,body_length);
     m->tlv.padn.type = 1;
     m->tlv.padn.length = length;
     memset(m->tlv.padn.MBZ,0x00,length);
}

void create_short_hello(dgc_packet* m)
{
     uint16_t body_length = 1+1+8;
     create_header(m,body_length);
     m->tlv.s_hello.type = 2;
     m->tlv.s_hello.length = 0x08;
     m->tlv.s_hello.src_id = MY_ID;
}

void create_long_hello(dgc_packet* m, uint64_t dst_id)
{
     uint16_t body_length = 1+1+16;
     create_header(m,body_length);
     m->tlv.l_hello.type = 2;
     m->tlv.l_hello.length = 0x10;
     m->tlv.l_hello.src_id = MY_ID;
     m->tlv.l_hello.dst_id = dst_id;
}

void create_neighbor(dgc_packet* m, unsigned char ipv6[], int port)
{
     uint16_t body_length = 1+1+16+2;
     create_header(m,body_length);
     m->tlv.neighbor.type = 3;
     m->tlv.neighbor.length = 0x12;
     memcpy(m->tlv.neighbor.ip,ipv6,16);
     m->tlv.neighbor.port = port;
}

void create_data(dgc_packet* m, uint8_t length, uint64_t sender_id, uint32_t nonce, uint8_t data_type, char* msg)
{
     uint16_t body_length = 1+1+13+length;
     create_header(m,body_length);
     m->tlv.data.type = 4;
     m->tlv.data.length = length + 13;
     m->tlv.data.sender_id = sender_id;
     m->tlv.data.nonce = nonce;
     m->tlv.data.data_type = data_type;
     memcpy(m->tlv.data.message, msg, length);
}

void create_ack(dgc_packet* m, uint64_t sender_id, uint32_t nonce)
{
     uint16_t body_length = 1+1+8+4;
     create_header(m,body_length);
     m->tlv.ack.type = 5;
     m->tlv.ack.length = 12;
     m->tlv.ack.sender_id = sender_id;
     m->tlv.ack.nonce = nonce;
}

void create_goaway(dgc_packet* m, uint8_t length, uint8_t code, char* msg)
{
     uint16_t body_length = 1+1+1+length;
     create_header(m,body_length);
     m->tlv.goaway.type = 6;
     m->tlv.goaway.length = length+1;
     m->tlv.goaway.code = code;
     memcpy(m->tlv.goaway.message, msg, length);
}

void create_warning(dgc_packet* m, uint8_t length, char* msg)
{
     uint16_t body_length = 1+1+length;
     create_header(m,body_length);
     m->tlv.warning.type = 7;
     m->tlv.warning.length = length;
     memcpy(m->tlv.warning.message, msg, length);
}
