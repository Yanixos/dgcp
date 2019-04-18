#ifndef HEXDUMP_H_INCLUDED
#define HEXDUMP_H_INCLUDED

#include <stddef.h>
#include <stdio.h>
#include <ctype.h>

static void hexdump(const void * memory, size_t bytes)
{
     const unsigned char * p, * q;
     int i;

     p = memory;
     int j=0;

     switch ( p[4] )
     {
          case 0 :
               printf("PAD1 TLV...\n");
               break;
          case 1 :
               printf("PADN TLV...\n");
               break;
          case 2 :
               if (p[5] == 8)
                    printf("SHORT HELLO TLV...\n");
               else if (p[5] == 16)
                    printf("LONG HELLO TLV...\n");
               else
                    printf("CORRUPTED HELLO TLV");
               break;
          case 3 :
               printf("NEIGHBOUR TLV...\n");
               break;
          case 4 :
               printf("DATA TLV...\n");
               break;
          case 5 :
               printf("ACK TLV...\n");
               break;
          case 6 :
               printf("GOAWAY TLV...\n");
               break;
          case 7 :
               printf("WARNING TLV...\n");
               break;
          default :
               printf("UNKNOWN TLV...\n");
               break;
     }

     while (bytes)
     {
          q = p;
          printf("%08X: ", j);

          for (i = 0; i < 16 && bytes; ++i)
          {
               printf("%02X ", *p);
               j += 1;
               ++p;
               --bytes;
          }

          bytes += i;

          while (i < 16)
          {
               printf("XX ");
               ++i;
          }

          printf("|");
          p = q;

          for (i = 0; i < 16 && bytes; ++i)
          {
               printf("%c", isprint(*p) && !isspace(*p) ? *p : '.');
               ++p;
               --bytes;
          }

          while (i < 16)
          {
               printf(" ");
               ++i;
          }
               printf("|\n");
     }
     return;
}

#endif
