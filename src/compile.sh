#!/bin/bash

gcc -o header.o -fPIC -c header.c -Wall
ar rcs header.a header.o
gcc -o dgcp_handler.o -fPIC -c dgcp_handler.c -Wall
ar rcs dgcp_handler.a dgcp_handler.o
gcc main.c -o main -lpthread -Wall ./header.a ./dgcp_handler.a
