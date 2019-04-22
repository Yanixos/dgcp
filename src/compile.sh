#!/bin/bash

gcc -o header.o -fPIC -c header.c -Wall
ar rcs header.a header.o
gcc main.c -o main -lpthread -Wall ./header.a
