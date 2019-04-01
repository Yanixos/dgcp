#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
##include "header.h"


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


uint64_t init_peer()
{
     unsigned char temp[SHA_DIGEST_LENGTH];
     char buf[SHA_DIGEST_LENGTH*2];

     memset(buf, 0x0, SHA_DIGEST_LENGTH*2);
     memset(temp, 0x0, SHA_DIGEST_LENGTH);

     char* mac = mac_eth0();
     SHA1((unsigned char *) mac, strlen(mac), temp);

     for (int i=0; i < SHA_DIGEST_LENGTH; i++)
         sprintf((char*)&(buf[i*2]), "%02x", temp[i]);

     return (uint64_t) buf;
}

int check_header(msg_hd header)
{
     return ( header.magic == 93 && header.version == 2 );
}

int msg_type(msg_packet m)
{
     if ( ! check_header(m.header) )
          return -1;

     else if ( m.tlv.type > 7 )
          return -1;
     else
          return m.tlv.type
}
