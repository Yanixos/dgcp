#ifndef NEIGHBORSCONTROLLER_H_INCLUDED
#define NEIGHBORSCONTROLLER_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include "header.h"

#define DATA_SIZE 256
#define RETRIES 5

recent_neighbors *MY_RN;
potential_neighbors *MY_PN;
data_array MY_DATA[DATA_SIZE];
int COUNT;

extern int search_id_RN(recent_neighbors *list,uint64_t id,recent_neighbors **current,recent_neighbors **precedent);
extern int search_key_RN(recent_neighbors *list,ip_port* key,ip_port **current,ip_port **precedent,uint64_t *id);
extern recent_neighbors* search_recent_neighbors_key(recent_neighbors *list,ip_port* key);
extern recent_neighbors* symetric_neighbors();
extern void create_recent_neighbor(uint64_t id,ip_port*  key,int symetric,time_t hello_t,time_t long_hello_t);
extern void delete_id_RN(recent_neighbors *list,uint64_t id);
extern void delete_key_RN(recent_neighbors *list,ip_port*  key);
extern void modify_hello(recent_neighbors *list,uint64_t id,time_t hello_t);
extern void modify_long_hello(recent_neighbors *list,uint64_t id,time_t long_hello_t);
extern void modify_symetric(recent_neighbors *list,uint64_t id,int symetric);
extern int search_id_PN(potential_neighbors *list,uint64_t id,potential_neighbors **current,potential_neighbors **precedent);
extern int search_key_PN(potential_neighbors *list,ip_port* key,ip_port **current,ip_port **precedent,uint64_t *id);
extern void create_potentiel_neighbor(uint64_t id, ip_port*  key);
extern void delete_key_PN(potential_neighbors *list,ip_port*  key);
extern int search_data(data_key key);
extern int add_data(data_key key, char* data,uint8_t type, recent_neighbors* data_neighbors);
extern void print_recent();
extern void print_potential();


#endif
