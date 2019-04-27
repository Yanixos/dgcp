#ifndef NEIGHBORSCONTROLLER_H_INCLUDED
#define NEIGHBORSCONTROLLER_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include "header.h"

#define TAILLE 256

recent_neighbors *MY_RN;
potential_neighbors *MY_PN;
data_array MY_DATA[TAILLE];
int COUNT = 0;
int POS = 0;

extern int search_id_RN(recent_neighbors *list,uint64_t id,recent_neighbors **current,recent_neighbors **precedent);
extern int search_key_RN(recent_neighbors *list,uint64_t id,ip_port* key,ip_port **current,ip_port **precedent);
extern recent_neighbors* symetric_neighbors();
extern void create_recent_neighbor(uint64_t id,ip_port*  key,int symetric,time_t hello_t,time_t long_hello_t);
extern void delete_id_RN(recent_neighbors *list,uint64_t id);
extern void delete_key_RN(recent_neighbors *list,uint64_t id,ip_port*  key);
extern void modify_hello(uint64_t id,time_t hello_t);
extern void modify_long_hello(uint64_t id,time_t long_hello_t);
extern void modify_symetric(uint64_t id,int symetric);
extern int search_id_PN(uint64_t id,potential_neighbors **current,potential_neighbors **precedent);
extern int search_key_PN(uint64_t id,ip_port* key,ip_port **current,ip_port **precedent);
extern void create_potentiel_neighbor(uint64_t id,ip_port*  key,int symetric,time_t hello_t,time_t long_hello_t);
extern int search_data(data_key key);
extern void add_data(data_key key,time_t data_time,char* data,potential_neighbors* data_neighbors,uint8_t nb);

#endif
