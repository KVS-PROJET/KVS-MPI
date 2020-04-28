#ifndef _KVS_H_
#define _KVS_H_

#include <stdio.h>
#include <stdlib.h>

struct element {
  char* key  ;
  char* value;
  struct element* Next ;
  struct element* Prev ; //
} ;

typedef struct element** KVS ; 

void Initialise_kvs(const int N ) ;
int kvs_get(char* key, char** out_data, size_t* data_size);
int kvs_put(char* key, char* in_data, size_t data_size);
int kvs_delete(char* key);
void Affichage();
void free_kvs();

#endif

