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

void build_data_type(MPI_Datatype *MPI_Key_vsize);

/*--------------------------------------- Clients side ----------------------------------------*/

int MPI_Client_set_data(char* key, char* value, size_t in_data, node_t* Server_ring_g, int limit_hash_space) ;

int MPI_Client_get_data(char* key, char** value, size_t* out_data, node_t* Server_ring_g, int limit_hash_space);

int MPI_Finish_task(int nbr_servers, int nproc, int target_client, int part);


/*-------------------------------------- Servers Side -------------------------------------------*/

/* initilize the ring of servers : processes with even rank (rank %2 = 0) */

void Initialize_ring(node_t** Server_ring_g   , int nbr_server, int nproc );

int MPI_Server_process_requests(local_kvs_t **local_kvs_root, double *elapsed_time, long int *nbr_opr);


void inorder_send(local_kvs_t** root, uint32_t, int id_piv, MPI_Datatype MPI_Key_vsize);

int mode_data(uint32_t s_src, uint32_t s_dest, local_kvs_t **local_kvs_root, int id_piv);

int MPI_Refresh_ring(node_t **Server_ring_g, int *msg_bcast, int chunk, int *nbr_servers, int part, local_kvs_t** local_kvs_root, int limit_hash_space);
#endif
