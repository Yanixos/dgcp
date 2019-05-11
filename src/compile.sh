#!/bin/bash

gcc -o header.o -fPIC -c header.c -Wall
ar rcs header.a header.o
gcc -o neighborsController.o -fPIC -c neighborsController.c -Wall
ar rcs neighborsController.a neighborsController.o
gcc -o dgcp_handler.o -fPIC -c dgcp_handler.c -lm  -Wall
ar rcs dgcp_handler.a dgcp_handler.o
gcc main.c -o main -g -lpthread -lm -Wall ./header.a ./dgcp_handler.a ./neighborsController.a
