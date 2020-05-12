#include <stdio.h>
#include <stdlib.h>
#include "murmur3.h"
#include <time.h>
#include <mpi.h>
#include <stdio.h>
#include <mpi_const_hash.h>
#include <local_kvs.h>
#include <Consistent_Hashing.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#define SET_TAG 1000
#define GET_TAG 2000

#define CHECK_SET_TAG 1111
#define CHECK_GET_TAG 2222

#define TAG_NOT_FOUND 88
#define TAG_FINISH    99

#define KEY_MAX 100
#define SEED 42

#define MOVE_TAG 55
#define END_MOVE 11


#if     UINTPTR_MAX == 0xffffffffffffffffULL
#define BUILD_64 1
#endif

void build_data_type(MPI_Datatype *MPI_Key_vsize){
        //MPI_Datatype MPI_Key_vsize; 
        int             blocklens[2] = {1, KEY_MAX};
        MPI_Aint        displacements[2] ;
        MPI_Aint        offsets[2];
        MPI_Datatype    typelist[2] = { MPI_UNSIGNED_LONG_LONG, MPI_CHAR};
        displacements[0] = offsetof(struct key_Vsize_s, size_value) ;
        displacements[1] = offsetof(struct key_Vsize_s, key) ;
        MPI_Type_create_struct(2, blocklens, displacements, typelist, MPI_Key_vsize);
        MPI_Type_commit(MPI_Key_vsize);
}



/* initilize the ring of servers : */
void Initialize_ring(node_t** Server_ring_g  , int nbr_servers, int nproc){	
	int chunk   =  100 / nbr_servers ;
	int offset  =  nproc / nbr_servers ;
	int cpt = 0;
       	uint32_t server_id = 0 ;
	int rank = 0 ;
	while(cpt < nbr_servers){
		*Server_ring_g = add_server(*Server_ring_g, rank, server_id);
		cpt ++;
		rank += offset;
		server_id += chunk;
	}
	//Server_BST_display(*Server_ring_g);
}

/*--------------------------------------- Clients side ----------------------------------------*/

int MPI_Client_set_data(char* key, char* value, size_t in_data, node_t* Server_ring_g, int limit_hash_space){
	MPI_Datatype MPI_Key_vsize;         
        build_data_type(&MPI_Key_vsize);
	int P, r;
	MPI_Comm_size(MPI_COMM_WORLD, &P);
	MPI_Comm_rank(MPI_COMM_WORLD, &r);
	
	int   put_checked;
	
	key_Vsize_t *to_send_1 = (key_Vsize_t *)malloc(sizeof(key_Vsize_t));
        strcpy(to_send_1->key , key);
	
	to_send_1->size_value  = in_data;	
	int t_value = strlen(value);

	Value_Buff_t    Value  = (Value_Buff_t )malloc( t_value * sizeof(Value_Buff_t));	
	strcpy(Value, value);	
	
	// Find the server on the ring which will store this Key-Value--------------------------
	uint32_t key_hash = 0;
	int len	= strlen(key);
	uint32_t seed = SEED;
	MurmurHash3_x86_32(key, len, seed, &key_hash);
	key_hash = key_hash % limit_hash_space ;	
	
	int target_server = 0 ; //
	int id_succ;
	find_successor(Server_ring_g, Server_ring_g, key_hash, &target_server, &id_succ);

	MPI_Send(to_send_1, 1, MPI_Key_vsize , target_server, SET_TAG, MPI_COMM_WORLD);
        MPI_Send(Value, to_send_1->size_value, MPI_CHAR, target_server, SET_TAG, MPI_COMM_WORLD);

	return 0 ;

} /* END of MPI_Client_set_data*/

int MPI_Client_get_data(char* key, char** value, size_t* out_data, node_t* Server_ring_g, int limit_hash_space){
	int key_l = strlen(key) + 1;
	MPI_Status stat;	
	int taille; 
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// Find the server on the ring which will lookup for the value-------------------------
	uint32_t key_hash = 0;
	int len	= strlen(key);
	uint32_t seed = SEED;
	MurmurHash3_x86_32(key, len, seed, &key_hash);

	int target_server = 0 ; //
	int id_succ;
	key_hash = key_hash % limit_hash_space ;
	find_successor(Server_ring_g, Server_ring_g, key_hash, &target_server, &id_succ);
	// -----------------------------------------------------------------------------------
	MPI_Request req[2] ;
	MPI_Status st;

	MPI_Send(key, key_l, MPI_CHAR, target_server, GET_TAG, MPI_COMM_WORLD) ;

/*	
	MPI_Probe(target_server, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
	if (stat.MPI_TAG == TAG_NOT_FOUND){
		MPI_Recv(NULL, 0, MPI_BYTE, target_server, TAG_NOT_FOUND, MPI_COMM_WORLD ,MPI_STATUS_IGNORE);
	}
       	else if (stat.MPI_TAG == CHECK_GET_TAG){
		MPI_Status stat_;
		MPI_Get_count(&stat_, MPI_BYTE, &taille);
		*value = (char *)malloc(taille * sizeof(char));
		MPI_Recv(*value, taille, MPI_CHAR, target_server, CHECK_GET_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		*out_data = taille ; // cast to size_t !
	}
*/
	return 0 ;

} /*  END of MPI_Client_get_data() 	*/

int MPI_Finish_task(int nbr_servers, int nproc, int target_client, int part){
	//int part = nproc / nbr_servers ;
	for(int i = 0 ; i < nproc; i++)
		if ( ( i % part == 0 ) || ( i == target_client) )
			MPI_Send(NULL, 0, MPI_BYTE, i, TAG_FINISH, MPI_COMM_WORLD);
	return 0;
}


/*-------------------------------------- Servers Side -------------------------------------------*/

int MPI_Server_process_requests(local_kvs_t **local_kvs_root, double *elapsed_time, long int *nbr_opr){
	int r;
	MPI_Comm_rank(MPI_COMM_WORLD, &r);

        MPI_Datatype MPI_Key_vsize; 
        build_data_type(&MPI_Key_vsize);

     	key_Vsize_t     to_receive1; // key , size_value
        Value_Buff_t    to_receive2;

        char* returned_value;
        char* to_recv;
        MPI_Status stat;
        int taille1 ;
        size_t taille_out ;
        int arrived = 0;
		
	double s_time = 0. , e_time = 0.;
	
	int finished = 1 ;
        do{
                MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);		
                int check_put, check_get ;
		
		s_time = MPI_Wtime();
		switch (stat.MPI_TAG){

                        case (SET_TAG) :

				*nbr_opr += 1 ;
                                MPI_Recv(&to_receive1 , 1, MPI_Key_vsize , stat.MPI_SOURCE , SET_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                                to_receive2 = (char*)malloc( to_receive1.size_value * sizeof(char));  
                                MPI_Recv(to_receive2, to_receive1.size_value, MPI_CHAR, stat.MPI_SOURCE, SET_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

			       	check_put = local_kvs_put(to_receive1.key, to_receive2, to_receive1.size_value, local_kvs_root);
				free(to_receive2);
                                break;
                        
			case (GET_TAG) :
				*nbr_opr += 1 ;
			       	MPI_Get_count(&stat, MPI_BYTE, &taille1);

				to_recv = (char *)malloc(taille1 * sizeof(char)); // free it!
                                
				MPI_Recv(to_recv , taille1 , MPI_CHAR , stat.MPI_SOURCE , GET_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                               
				returned_value = NULL;
				taille_out = 0;
				check_get = local_kvs_get(to_recv, &returned_value, &taille_out, *local_kvs_root);

				/*if(check_get == 0){
					MPI_Send(returned_value, taille_out + 1, MPI_CHAR, stat.MPI_SOURCE, CHECK_GET_TAG, MPI_COMM_WORLD) != 0 ;
                                }
                                else {
					MPI_Send(NULL, 0, MPI_BYTE, stat.MPI_SOURCE, TAG_NOT_FOUND, MPI_COMM_WORLD) ;
                               	}	
                                */
				break;

                        case (TAG_FINISH) :
                                MPI_Recv(NULL, 0, MPI_BYTE, MPI_ANY_SOURCE, TAG_FINISH, MPI_COMM_WORLD ,MPI_STATUS_IGNORE);
                                finished = 0 ;
				break;
                               
		}
		e_time = MPI_Wtime();
		*elapsed_time += e_time - s_time;
	
	}while(finished);	


} /*  END of  MPI_Server_process_requests() */


/* functions to update dynamically the ring and move the prospective data */

void inorder_send(local_kvs_t** root, uint32_t dest, int id_piv, MPI_Datatype MPI_Key_vsize){
	if(*root == NULL){
		MPI_Send(NULL, 0, MPI_BYTE, dest, END_MOVE, MPI_COMM_WORLD);
		return;
	}
	else{
		inorder_send( &((*root)->left) , dest, id_piv, MPI_Key_vsize );
		
		if ( (*root)->key_hash <= id_piv ){
			
			key_Vsize_t *to_send_1 = (key_Vsize_t *)malloc(sizeof(key_Vsize_t));
        	
			strcpy(to_send_1->key , (*root)->key);
			size_t size = (*root)->key_hash ;
			to_send_1->size_value  = size;	
			Value_Buff_t    Value  = (Value_Buff_t )malloc(size * sizeof(Value_Buff_t));
			strcpy(Value, (*root)->value);	
		
			MPI_Send(to_send_1, 1, MPI_Key_vsize , dest, MOVE_TAG, MPI_COMM_WORLD);
        		MPI_Send(Value, to_send_1->size_value, MPI_CHAR, dest, MOVE_TAG, MPI_COMM_WORLD);
		}

		inorder_send( &((*root)->right), dest, id_piv, MPI_Key_vsize );
	}

}

int move_data(uint32_t s_src , uint32_t s_dest, local_kvs_t** local_kvs_root, int id_piv){
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Datatype MPI_Key_vsize ;
	build_data_type(&MPI_Key_vsize);

	if (rank == s_src){
		inorder_send(local_kvs_root, s_dest, id_piv, MPI_Key_vsize);
	}
	else if (rank == s_dest){
		MPI_Status stat;
		int fin = 1;
		do{
			MPI_Probe(s_src, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
			key_Vsize_t     to_receive1; // key , size_value
        		Value_Buff_t    to_receive2;
			switch(stat.MPI_TAG){
				case MOVE_TAG :
					MPI_Recv(&to_receive1 , 1, MPI_Key_vsize , stat.MPI_SOURCE , MOVE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                                	to_receive2 = (char*)malloc( to_receive1.size_value * sizeof(char));  
                                	MPI_Recv(to_receive2, to_receive1.size_value, MPI_CHAR, stat.MPI_SOURCE, MOVE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

					int check_put = local_kvs_put(to_receive1.key, to_receive2, to_receive1.size_value, local_kvs_root);
					break;
				case END_MOVE :
					MPI_Recv(NULL, 0, MPI_BYTE, stat.MPI_SOURCE, END_MOVE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					fin = 0 ;
					break;
			}
		}while(fin);
	}
}

int MPI_Refresh_ring (node_t** Server_ring_g, int* msg_bcast, int chunk, int *nbr_servers, int part, local_kvs_t** local_kvs_root, int limit_hash_space){
	int tag = msg_bcast[0];
	int next_id, prev_id;
	int id_piv;
	uint32_t s_src, s_dest ;
	int rank_src, rank_dest;
	int id_succ, tmp_id_prev;	
	switch(tag){
		case 1 :			
			*nbr_servers += 1;
			int new_server 	= msg_bcast[1];
			int tmp_id	= new_server % part ;
			if(tmp_id != 0){
				int tmp_prev 	= new_server - tmp_id ;
				tmp_id_prev = (tmp_prev / part) * chunk + 1;
			       	do{
					find_successor(*Server_ring_g, *Server_ring_g, tmp_id_prev, &next_id, &id_succ);
					if(next_id  > new_server)
						break;
					else
						tmp_id_prev = id_succ ;
	
				}while(1);	
			}

			int bord;
			prev_id = tmp_id_prev - 1;
			if( id_succ != 0)
				bord	= id_succ ;
			else
				bord	= limit_hash_space ;
			
			int new_id = (bord + prev_id) / 2 ;
			*Server_ring_g = add_server(*Server_ring_g, new_server, new_id);

			// set parameters to move data
			s_src 	= next_id;
			s_dest 	= new_server ;
			id_piv = new_id; // we'll depalce just data which their hash key <= new_id !
			break ;
		case -1 :
			*nbr_servers -= 1;
			int to_remove 	= msg_bcast[1];
			node_t* target_node = NULL;
			search_server(*Server_ring_g, to_remove, &target_node);
			int id_toremove;
			if(target_node != NULL)
				id_toremove	= target_node -> node_ID;
			else {
				perror("Search_server !");
				return 1 ;
			}
			*Server_ring_g = remove_server(*Server_ring_g, id_toremove);
			int id_succ ;
			find_successor(*Server_ring_g, *Server_ring_g, id_toremove, &next_id, &id_succ);			
			// set parameters to move data
			s_src 	= to_remove;
			s_dest	= next_id;
			id_piv = INT_MAX; // because we'll deplace all data from src to sest 
			break ;
	}

	move_data(s_src, s_dest, local_kvs_root , id_piv);
	return 0;
}
