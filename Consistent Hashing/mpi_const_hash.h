#ifndef _MPI_CONST_HASH_H
#define _MPI_CONST_HASH_H
#include <stdio.h>
#include <stdlib.h>
#include <local_kvs.h>
#include <stddef.h>
#include <stdint.h>
#include <Consistent_Hashing.h>
#define KEY_MAX 100

typedef struct key_Vsize_s{
	size_t size_value ;
	char key[KEY_MAX] ;
} key_Vsize_t;

typedef char* Value_Buff_t  ;


/*--------------------------------------- Clients side ----------------------------------------*/

int MPI_Client_set_data(char* key, char* value, size_t in_data, node_t* Server_ring_g) ;

int MPI_Client_get_data(char* key, char** value, size_t* out_data, node_t* Server_ring_g);

int MPI_Finish_task();


/*-------------------------------------- Servers Side -------------------------------------------*/

/* initilize the ring of servers : processes with even rank (rank %2 = 0) */
int MPI_Initilize_server_ring(local_kvs_t* local_kvs_root/*, int rank, node_t** Server_ring_g*/);
void Initialize_ring(node_t** Server_ring_g   /*, int *glob */);

int MPI_Server_process_requests(local_kvs_t *local_kvs_root);

#endif
