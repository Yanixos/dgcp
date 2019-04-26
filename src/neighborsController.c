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
  recent_neighbors *tmp1,*tmp2;
  //neighbor exist deja
  if(search_id(id,&tmp1,&tmp2)){
    ip_port *tmp11,*tmp22;
    //key exist deja
    if(search_key(tmp1,&tmp11,&tmp22)){
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

void delete_id(uint64_t id){
  recent_neighbors *current,*precedent;
  if(search_id(id,&current,&precedent)){
    //la list contient un seul element
    if(precedent!=NULL)
      MY_RN=NULL;
    else
      precedent->next=current->next;
    free(current);
  }
}

void delete_key(uint64_t id,ip_port*  key){
  ip_port *current,*precedent;
  if(search_key(id,key,&current,&precedent)){
    //la list contient un seul element
    if(precedent!=NULL)
      delete_id(id);
    else
      precedent->next=current->next;
    free(current);
  }
}