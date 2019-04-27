#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "neighborsController.h"

/*========= Function for RN =========*/

int search_id_RN(recent_neighbors *list,uint64_t id,recent_neighbors **current,recent_neighbors **precedent){
    *current = list;
    *precedent=NULL;
    while (*current != NULL)
    {
        if ((*current)->id == id)
            return 1;
        *precedent=*current;
        *current = (*current)->next;
    }
    return 0;
}

int equal_key(recent_neighbors list,ip_port *key1, ip_port *key2){
   return (memcmp(key1->ip, key2->ip, 16) == 0 ) & (key1->port == key2->port)
}

//recherche sans id
int search_key_RN(recent_neighbors *list,ip_port* key,ip_port **current,ip_port **precedent,uint64_t *id){
  recent_neighbors *tmp1=list,*tmp2=NULL;
  while(tmp1!=NULL){
    id=tmp1;
    *current = tmp1->key;
    *precedent=NULL;
    while (*current != NULL)
    {
        if (equal_key(key,*current))
            return 1;
        *precedent=*current;
        current = current->next;
    }
    tmp2=tmp1;
    tmp1=tmp1->next;
  }
  return 0;
}

recent_neighbors* search_recent_neighbors_key(recent_neighbors *list,ip_port* key){
  ip_port *tmp11=list,*tmp22=NULL;
  uint64_t tmp;
  if(search_key_RN(list,key,&tmp11,&tmp22,&tmp)){
    if(search_id_RN(list,tmp,&tmp1,&tmp2))
      return tmp1;
  }
  return NULL;
}

//creeer nouvelle list
recent_neighbors* symetric_neighbors(){

  recent_neighbors *tmp,*result=NULL;
  while(tmp!=NULL){
    if(tmp->symetric==1){
      recent_neighbors *rn;
      rn= (recent_neighbors *) calloc(1,sizeof(recent_neighbors));
      rn->id = tmp->id;
      rn->key = (ip_port*) calloc(1,sizeof(ip_port));
      memcpy(rn->key->ip,tmp->key->ip,16);
      rn->key->port = tmp->key->port;
      rn->key->next = NULL;
      rn->symetric = tmp->symetric;
      rn->hello_t = tmp->hello_t;
      rn->long_hello_t = tmp->long_hello_t;
      rn->next = NULL;
      if(result!=NULL)
        rn->next=result;
      result=rn;

    }
  }
  return result;
}

void create_recent_neighbor(uint64_t id,ip_port*  key,int symetric,time_t hello_t,time_t long_hello_t){
  recent_neighbors *tmp1,*tmp2;
  uint64_t tmp;
  //neighbor exist deja
  if(search_id_RN(MY_RN,id,&tmp1,&tmp2)){
    ip_port *tmp11,*tmp22;
    //key exist deja
    if(search_key_RN(MY_RN,tmp1,&tmp11,&tmp22,&tmp)){
        return;
    }
    else{
        //ajout d'une nouvelle key
        if(tmp1->key !=NULL)
          //ajout au debut de la list
          key->next=tmp1->key;
        tmp1->key=key;
    }
  }
  else{
    //creation de l'element
    recent_neighbors* rn= (recent_neighbors *) calloc(1,sizeof(recent_neighbors));
    rn->id = id;
    rn->key = (ip_port*) calloc(1,sizeof(ip_port));
    memcpy(rn->key->ip,key->ip,16);
    rn->key->port = port;
    rn->key->next = NULL;
    rn->symetric = symetric;
    rn->hello_t = hello_t;
    rn->long_hello_t = long_hello_t;
    rn->next = NULL;

    //ajout d'un nouveau id
    if(MY_RN != NULL)
      //ajout au debut de la list
      rn->next=MY_RN;

    MY_RN=rn;

  }
}

void delete_id_RN(recent_neighbors *list,uint64_t id){
  recent_neighbors *current,*precedent;
  if(search_id_RN(list,id,&current,&precedent)){
    //la list contient un seul element
    if(precedent!=NULL)
      list=NULL;
    else
      precedent->next=current->next;
    free(current);
  }
}


void delete_key_RN(recent_neighbors *list,ip_port*  key){
  ip_port *current,*precedent;
  uint64_t tmp;
  if(search_key_RN(list,key,&current,&precedent,&tmp)){
    //la list contient un seul element
    if(precedent!=NULL)
      delete_id_RN(tmp);
    else
      precedent->next=current->next;
    free(current);
  }
}

void modify_hello(uint64_t id,time_t hello_t){
  recent_neighbors *current,*precedent;
  if(search_id_RN(id,&current,&precedent)){
    current->hello_t=hello_t;
  }
}

void modify_long_hello(uint64_t id,time_t long_hello_t){
  recent_neighbors *current,*precedent;
  if(search_id_RN(id,&current,&precedent)){
    current->long_hello_t=long_hello_t;
  }
}

void modify_symetric(uint64_t id,int symetric){
  recent_neighbors *current,*precedent;
  if(search_id_RN(id,&current,&precedent)){
    current->symetric=symetric;
  }
}
/*========= Function for PN =========*/

int search_id_PN(potential_neighbors *list,uint64_t id,potential_neighbors **current,potential_neighbors **precedent){
    *current = list;
    *precedent=NULL;
    while (*current != NULL)
    {
        if ((*current)->id == id)
            return 1;
        *precedent=*current;
        current = current->next;
    }
    return 0;
}



int search_key_PN(potential_neighbors *list,ip_port* key,ip_port **current,ip_port **precedent,uint64_t *id){
  potential_neighbors *tmp1=list,*tmp2=NULL;

int search_key_PN(ip_port* key,ip_port **current,ip_port **precedent){
  potential_neighbors *tmp1,*tmp2;
  if(search_id_PN(id,&tmp1,&tmp2)){
int search_key_PN(uint64_t id,ip_port* key,ip_port **current,ip_port **precedent,uint64_t *id){
  potential_neighbors *tmp1=MY_PN,*tmp2=NULL;
  while(tmp1!=NULL){
    id=tmp1;
    *current = tmp1->key;
    *precedent=NULL;
    while (*current != NULL)
    {
        if (equal_key(key,*current))
            return 1;
        *precedent=*current;
        current = current->next;
    }
    tmp2=tmp1;
    tmp1=tmp1->next;
  }
  return 0;
}


void create_potentiel_neighbor(uint64_t id, ip_port*  key){
  potential_neighbors *tmp1,*tmp2;
  uint64_t tmp;
  //neighbor exist deja
  if(search_id_PN(id,&tmp1,&tmp2)){
    ip_port *tmp11,*tmp22;
    //key exist deja
    if(search_key_PN(tmp1,&tmp11,&tmp22,&tmp)){
        return;
    }
    else{
        //ajout d'une nouvelle key
        if(tmp1->key !=NULL)
          //ajout au debut de la list
          key->next=tmp1->key;
        tmp1->key=key;
    }
  }
  else{
    //creation de l'element
    potential_neighbors* rn= (recent_neighbors *) calloc(1,sizeof(recent_neighbors));
    rn->id = id;
    rn->key = (ip_port*) calloc(1,sizeof(ip_port));
    memcpy(rn->key->ip,key->ip,16);
    rn->key->port = port;
    rn->key->next = NULL;
    rn->next = NULL;

    //ajout d'un nouveau id
    if(MY_PN != NULL)
      //ajout au debut de la list
      rn->next=MY_PN;

    MY_PN=rn;

  }
}

void delete_id_PN(potential_neighbors *list,uint64_t id){
  potential_neighbors *current,*precedent;
  if(search_id_PN(list,id,&current,&precedent)){
    //la list contient un seul element
    if(precedent!=NULL)
      list=NULL;
    else
      precedent->next=current->next;
    free(current);
  }
}

void delete_key_PN(potential_neighbors *list,ip_port*  key){
  ip_port *current,*precedent;
  uint64_t tmp;
  if(search_key_PN(list,key,&current,&precedent,&tmp)){
    //la list contient un seul element
    if(precedent!=NULL)
      delete_id_PN(tmp);
    else
      precedent->next=current->next;
    free(current);
  }
}
/*========= Function for DATA =========*/

int equal_key_data(data_key *key1, data_key *key2){
   return (key1->id == key2->id) & (key1->nonce == key2->nonce)
}

int search_data(data_key key){

    for(int i=0,i<DATA_SIZE,i++)
    {
        if (equal_key_data(key,current->key))
            return i;
    }
    return -1;
}

int add_data(data_key key,char* data, uint8_t type, recent_neighbors* data_neighbors){

  //neighbor exist deja
  if(search_data(key)==-1){
    //creation de l'element
    data_array* elem= (data_array *) calloc(1,sizeof(data_array));
    elem->key = key;
    memcpy(elem->data,data,strlen(data));
    elem->data_neighbors=data_neighbors;

    //ajout d'un nouveau data
    if(COUNT==DATA_SIZE)
      COUNT=0;
    MY_DATA[COUNT]=elem;
    int r = COUNT;
    COUNT++;
    return r;
  }
}
