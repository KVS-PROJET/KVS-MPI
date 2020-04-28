#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "kvs.h"
#include "murmur3.h"
#include <pthread.h>
#include <assert.h>


uint32_t taille_kvs = 0;

KVS kvs = NULL ;

void insert_key_value(struct element* kvs, char* key, char* in_data, struct element* _next, struct element* _prev){
      kvs -> key   = 	key ;
      kvs -> value = 	in_data ;
      kvs -> Next  = 	_next ;
      kvs -> Prev  = 	_prev ;
}

static pthread_mutex_t mut_init = PTHREAD_MUTEX_INITIALIZER ;
static pthread_mutex_t mut_set	= PTHREAD_MUTEX_INITIALIZER ;

void Initialise_kvs (const int N) {
 	 //printf("%s\n" , "Initialisation");
  	pthread_mutex_lock(&mut_init);
  	taille_kvs = N;
  	kvs = malloc(sizeof(struct element *) * N);
  	for (int i =0; i<N; ++i){
    		kvs[i] = NULL ;
  	}
  	pthread_mutex_unlock(&mut_init);
}


int kvs_get(char* key, char** out_data, size_t* data_size){
  
 	 uint32_t index = 0;
  	int len = strlen(key);
  	uint32_t seed = 42 ;
  	MurmurHash3_x86_32(key , len, seed, &index);
  
  	if (index > taille_kvs)
    		index %= taille_kvs ;
  
  	struct element * ptr = kvs[index]   ;

  	struct element * ptr_prev ;
  	if (ptr == NULL){
    		return 1 ;
  	}
  	while( ptr != NULL){
    		if (strcmp(ptr->key , key) == 0){
       			*out_data = ptr->value;
      			*data_size  =  strlen(*out_data);
      			//printf("%ld\n" , *data_size);
      			return 0 ;
    		}
    		ptr_prev = ptr ;
    		ptr = ptr_prev -> Next ;
  	}

  	return 1 ;
}



//pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER ;

int kvs_put(char* key, char * in_data, size_t data_size){
	uint32_t index = 0;
  	int len = strlen(key);
  	uint32_t seed = 42 ;
  	MurmurHash3_x86_32(key , len, seed, &index);
  
  	pthread_mutex_lock(&mut_set);

  	if (index > taille_kvs)
    	index %= taille_kvs ;

  	if ( kvs[index] == NULL ){
    	/* insert without collision : the table entry corresponding to this index is empty*/	  
    	//printf("%s\n", "ajout sans collision");
    	kvs[index] = malloc(sizeof(struct element));
    	insert_key_value(kvs[index] , key, in_data, NULL, NULL /*to update prev*/);
    	pthread_mutex_unlock(&mut_set);
    	
	return 1 ;
  	}
  	else {
    		/* if the key already existed : just update the value */		 

 		struct element *ptr = kvs[index] ;
 		struct element *tmp ;

   		while( ptr != NULL){
	    		if (strcmp(ptr->key , key) == 0){
				kvs[index]-> value = in_data ;
				pthread_mutex_unlock(&mut_set);
       	    			return 1 ;
    	    	}
            	tmp = ptr ;
    	    	ptr = tmp -> Next ;
   	}

    	/* if not insert the pair key-value*/
    	//printf("%s\n", "ajout avec collision");
    	ptr = kvs[index] ;
    	struct element * new_ptr = malloc(sizeof(struct element*));
    	//printf("%ln" , ptr);
    	insert_key_value(new_ptr , key, in_data, ptr, ptr->Prev) ;
    	kvs[index] = new_ptr ;
    	pthread_mutex_unlock(&mut_set);
    	return 1 ;
  	}
  	//pthread_mutex_unlock(&mut);

  	return 0 ;
}

int kvs_delete(char* key){
       	uint32_t index = 0;
	int len = strlen(key);
	uint32_t seed = 42 ;
  	MurmurHash3_x86_32(key , len, seed, &index);
  	
	if(kvs[index] == NULL)
		return 1; // key doesn't exist
	
	// find entry 
	struct element *ptr = kvs[index] ;
	struct element *tmp;
	while( ptr != NULL){
    		if (strcmp(ptr->key , key) == 0){
       			( ptr -> Prev ) -> Next = ptr -> Next ;
			( ptr -> Next ) -> Prev = ptr -> Prev ;
			assert(ptr->key != NULL); 
			assert(ptr->value != NULL);
			free(ptr->key);
			free(ptr->value);
			free(ptr);	
      		return 0 ;
   		 }
    		tmp = ptr ;
    		ptr = tmp -> Next ;
  	}	

	return 1 ;
}

void Affichage(){
  
	for (uint32_t i =0; i<taille_kvs; ++i){
    		if (kvs[i] == NULL)
      			continue;
    		printf("kvs[%d] = %s : %s   | ", i, (kvs[i])->key, (kvs[i])->value);
    		struct element* ptr = (kvs[i])->Next;
    		//printf("%p", ptr);
    		//printf("%s : %s", ptr->key, ptr->value);
    		while (ptr != NULL){
      			printf( "%s : %s   |", ptr->key, ptr->value );
      			ptr = ptr->Next;
    		}
    		printf("\n");
  	}
}

void free_kvs(){
  free(kvs);
}
