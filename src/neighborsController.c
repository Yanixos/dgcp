recent_neighbors *MY_RN;


int search_id(uint64_t id,recent_neighbors **current,recent_neighbors **precedent){
    *current = MY_RN;
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

int equal_key(ip_port *key1, ip_port *key2){
   return (memcmp(key1->ip, key2->ip, 16) == 0 ) & (key1->port == key2->port)
}

int search_key(uint64_t id,ip_port* key,ip_port **current,ip_port **precedent){
  recent_neighbors *tmp1,*tmp2;
  if(search_id(id,&tmp1,&tmp2)){
    *current = tmp1->key;
    *precedent=NULL;
    while (*current != NULL)
    {
        if (equal_key(key,*current))
            return 1;
        *precedent=*current;
        current = current->next;
    }
  }
  return 0;
}

void create_neighbor(uint64_t id,ip_port*  key,int symetric,time_t hello_t,time_t long_hello_t){

}

void delete_id(uint64_t id){

}

void delete_key(uint64_t id,ip_port*  key){

}
