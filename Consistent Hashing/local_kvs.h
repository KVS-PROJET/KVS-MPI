#ifndef _LOCAL_KVS_H
#define _LOCAL_KVS_H
#include <stdio.h>
#include <stdlib.h>
#include "murmur3.h"
#include <string.h>
#include <stddef.h>
#include <stdint.h>

typedef struct local_kvs_s{
        uint32_t key_hash ;
        char* key       ;
        char* value     ;
        struct local_kvs_s *left;
        struct local_kvs_s *right;
} local_kvs_t ;



// allocate binary search structure to store key-value locally in servers
local_kvs_t* make_local_kvs(uint32_t key_hash, char* key, char* value);


/* add an entry to the local kvs*/
local_kvs_t* add_entry(local_kvs_t* root, uint32_t key_hash, char* key, char* value);


/* find the smallest key in the local kvs */
int find_low_local_key(local_kvs_t* root, local_kvs_t** target_entry);


/* remove a key from a local kvs*/
local_kvs_t* remove_local_key(local_kvs_t* root, uint32_t to_remove);


/*searching for a key in a local kvs*/
int search_key(local_kvs_t* root, uint32_t key_, local_kvs_t ** target_entry);


int MPI_Initilize_local_kvs(local_kvs_t* root);


int local_kvs_put(char* local_key, char* local_value, size_t size_data, local_kvs_t** local_kvs_root);


int local_kvs_get(char* local_key, char** local_value, size_t* size_data, local_kvs_t* local_kvs_root);


/* inorder traversal */
void inorder(local_kvs_t* root);

#endif

