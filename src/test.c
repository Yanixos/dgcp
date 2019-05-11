#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
     int opt,myport,hisport;
     char* hostname;

     if ( argc != 7 || strcmp(argv[1],"-b") || strcmp(argv[3],"-h") || strcmp(argv[5],"-p") )
     {
          fprintf(stderr, "Usage: %s -b 'binding port' -h 'neighbor hostname/IPv4/IPv6' -p 'neighbor port'\n",argv[0] );
          exit(EXIT_FAILURE);
     }


     while((opt = getopt(argc, argv, "b:h:p:")) != -1)
     {
          switch(opt)
          {
               case 'b':
                    myport = atoi(optarg);
                    break;
               case 'h':
                    hostname = optarg;
               case 'p':
                    hisport = atoi(optarg);
                    break;
          }
     }

     return 0;
}
