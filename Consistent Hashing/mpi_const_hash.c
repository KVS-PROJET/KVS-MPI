#include <stdio.h>
#include <stdlib.h>
#include "murmur3.c"
#include <time.h>
#include <mpi.h>
#include <stdio.h>
#include <mpi_const_hash.h>
#include <local_kvs.h>
#include <Consistent_Hashing.h>
#include <stddef.h>
#include <stdint.h>

#define SET_TAG 1000
#define GET_TAG 2000

#define CHECK_SET_TAG 1111
#define CHECK_GET_TAG 2222

#define TAG_NOT_FOUND 88
#define TAG_FINISH    99

#define KEY_MAX 100
#define SEED 42

#if     UINTPTR_MAX == 0xffffffffffffffffULL
#define BUILD_64 1
#endif




/* initilize the ring of servers : processes with even rank (rank %2 = 0) */

int MPI_Initilize_server_ring(local_kvs_t* local_kvs_root/*, int rank, node_t** Server_ring_g*/){
	
	// node_t* Server_ring_g = NULL; // this BST structure will contain all servers distributed by murmurhash function
/*	if (*Server_ring_g != NULL)
		*Server_ring_g = NULL;

	//puts("-----");
	// server
	//Find the hash value of this server by hashing the string : "server with rank - "		
	uint32_t server_hash_id	= 0;
	char server [100];
	//printf("server = %s\n", server);	
	sprintf(server, "server with rank %d", rank);

	//printf("server = %s length = %ld\n", server, strlen(server));
	int len	= strlen(server);
	uint32_t seed = SEED;
	MurmurHash3_x86_32(server, len, seed, &server_hash_id);
	//printf("rank %d : hash server = %ud\n",rank, server_hash_id);
		
	// Add server to the ring		
	*Server_ring_g	=   add_server(*Server_ring_g, rank, server_hash_id) ;
*/
	//printf("Server_ring = %p\n", *Server_ring_g);
	// Initilize a local kvs on this server :
	MPI_Initilize_local_kvs(local_kvs_root);
	//printf("Local_kvs_root = %p\n", local_kvs_root);
	// local_kvs_root == NULL ;
		
	return 0;		
}

void Initialize_ring(node_t** Server_ring_g  /*, int *glob */){
	//printf("&glob = %p\n", glob);
	
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	for(int rank = 0; rank < size; rank++){
		if ( rank %2 == 0){
			if (*Server_ring_g != NULL)
				*Server_ring_g = NULL;

			//puts("-----");
			// server
			//Find the hash value of this server by hashing the string : "server with rank - "		
			uint32_t server_hash_id	= 0;
			char server [100];
			//printf("server = %s\n", server);	
			sprintf(server, "server with rank %d", rank);

			//printf("server = %s length = %ld\n", server, strlen(server));
			int len	= strlen(server);
			uint32_t seed = SEED;
			MurmurHash3_x86_32(server, len, seed, &server_hash_id);
			//printf("rank %d : hash server = %ud\n",rank, server_hash_id);
		
			
			//*glob = rank ; // to test global variabl..
			
			// Add server to the ring		
			*Server_ring_g = add_server(*Server_ring_g, rank, server_hash_id) ;		
		}
	}
}





/*--------------------------------------- Clients side ----------------------------------------*/

int MPI_Client_set_data(char* key, char* value, size_t in_data, node_t* Server_ring_g){
	puts("#####");
	//const int key_l   = strlen(key) + 1;
	// Build datatype and set data to be sent ----------------------------------------------
	MPI_Datatype MPI_Key_vsize;         
        int             blocklens[2] = {1, KEY_MAX};
        MPI_Aint        displacements[2] ;
        MPI_Aint        offsets[2];        
        // check the compiler first : size_t might be unsigned long or unsigned long long !
        MPI_Datatype    typelist[2] = { MPI_UNSIGNED_LONG_LONG, MPI_CHAR};
        displacements[0] = offsetof(struct key_Vsize_s, size_value) ;
        displacements[1] = offsetof(struct key_Vsize_s, key) ;  
        MPI_Type_create_struct(2, blocklens, displacements, typelist, &MPI_Key_vsize);
        MPI_Type_commit(&MPI_Key_vsize);

	int   put_checked;
	key_Vsize_t *to_send_1 = (key_Vsize_t *)malloc(sizeof(key_Vsize_t));
        strcpy(to_send_1->key , key); // const /
	to_send_1->size_value  = in_data;	
	int t_value = strlen(value); // + 1
	Value_Buff_t    Value  = (Value_Buff_t )malloc( t_value * sizeof(Value_Buff_t));	
	strcpy(Value, value);	
	//--------------------------------------------------------------------------------------
	// Find the server on the ring which will store this Key-Value--------------------------
	uint32_t key_hash = 0;
	int len	= strlen(key);
	uint32_t seed = SEED;
	MurmurHash3_x86_32(key, len, seed, &key_hash);

	int target_server = 0 ; //
	find_successor(Server_ring_g, key_hash, &target_server);
	printf("target_server = %d\n", target_server);
	//--------------------------------------------------------------------------------------
	
	// The target server found, then we send the data to be sent as usual ------------------
	MPI_Send(to_send_1, 1, MPI_Key_vsize , target_server, SET_TAG, MPI_COMM_WORLD);
        MPI_Send(Value, to_send_1->size_value, MPI_CHAR, target_server, SET_TAG, MPI_COMM_WORLD);
        
	// Waiting until the set was successful (put_checked = 0) or not ( = 1 )!
	MPI_Recv(&put_checked, 1, MPI_INT, target_server, CHECK_SET_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	
	return 0 ;
} /* END of MPI_Client_set_data*/

int MPI_Client_get_data(char* key, char** value, size_t* out_data, node_t* Server_ring_g){
	int key_l = strlen(key) + 1;
	MPI_Status stat;	
	int taille; 
	// Find the server on the ring which will lookup for the value-------------------------
	uint32_t key_hash = 0;
	int len	= strlen(key);
	uint32_t seed = SEED;
	MurmurHash3_x86_32(key, len, seed, &key_hash);

	int target_server = 0 ; //
	find_successor(Server_ring_g, key_hash, &target_server);
	// -----------------------------------------------------------------------------------
		
	if( MPI_Send(key, key_l, MPI_CHAR, target_server, GET_TAG, MPI_COMM_WORLD) != 0){
		perror("MPI_get_data");
		return 1 ;
	}
	
	MPI_Probe(target_server, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);

	if (stat.MPI_TAG == TAG_NOT_FOUND){           
	       	MPI_Recv(NULL, 0, MPI_BYTE, target_server, TAG_NOT_FOUND, MPI_COMM_WORLD ,MPI_STATUS_IGNORE);
                //puts("KEY NOT FOUND IN KVS FROM PROCESSUS %ld", target_server);
        }

       	if (stat.MPI_TAG == CHECK_GET_TAG){
		//puts("-------------");
		MPI_Get_count(&stat, MPI_BYTE, &taille);
		*value = (char *)malloc(taille * sizeof(char)); // !
		MPI_Recv(*value, taille, MPI_CHAR, target_server, CHECK_GET_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		*out_data = taille ; // cast to size_t !
		//printf("MPI Get answer %s\n", get_value);
	 }

	return 0 ;
} /*  END of MPI_Client_get_data() 	*/

int MPI_Finish_task(int P){
	for(int i = 0 ; i < P; i++)
		if (i %2 == 0)
			MPI_Send(NULL, 0, MPI_BYTE, i, TAG_FINISH, MPI_COMM_WORLD);
	return 0;
}




/*-------------------------------------- Servers Side -------------------------------------------*/

int MPI_Server_process_requests(local_kvs_t *local_kvs_root){

     	key_Vsize_t     to_receive1; // key , size_value
        Value_Buff_t    to_receive2;

        char* returned_value;
        char* to_recv;
        MPI_Status stat;
        int taille1 ;
        size_t taille_out ;
        int arrived = 0;
        MPI_Datatype MPI_Key_vsize; 
        
        int             blocklens[2] = {1, KEY_MAX};    
        MPI_Aint        displacements[2] ;
        MPI_Aint        offsets[2];
        // check the compiler first : size_t might be unsigned long or unsigned long long !
        MPI_Datatype    typelist[2] = { MPI_UNSIGNED_LONG_LONG, MPI_CHAR};
        displacements[0] = offsetof(struct key_Vsize_s, size_value) ;
        displacements[1] = offsetof(struct key_Vsize_s, key) ;  
        MPI_Type_create_struct(2, blocklens, displacements, typelist, &MPI_Key_vsize);
        MPI_Type_commit(&MPI_Key_vsize);

	/* get hash of this_server on the ring ! */
		
	int finished = 0 ; // changed only in default switch statement 
        do{

                MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);

                int check_put, check_get ;

                switch (stat.MPI_TAG){

                        case (SET_TAG) :
                                MPI_Recv(&to_receive1 , 1, MPI_Key_vsize , stat.MPI_SOURCE , SET_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                                to_receive2 = (char*)malloc( to_receive1.size_value * sizeof(char));  
                                MPI_Recv(to_receive2, to_receive1.size_value, MPI_CHAR, stat.MPI_SOURCE, SET_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                                //puts("IN : SET___________TAG");

                                //printf("data : %s\t%ld\t%s\n", to_receive1.key, to_receive1.size_value, to_receive2);
                                check_put = local_kvs_put(to_receive1.key, to_receive2, to_receive1.size_value, &local_kvs_root);

                                //printf("to set in kvs : %s\n", to_receive1.key);
                                MPI_Send(&check_put, 1, MPI_INT, stat.MPI_SOURCE, CHECK_SET_TAG, MPI_COMM_WORLD);
                                //printf("%d : sent ok  \n",check_put);
                                //puts("Sending status");
                                free(to_receive2);
                                break;
                        case (GET_TAG) :
                                // Receiving msg if MPI_Probe correspond to kvs_get
                                MPI_Get_count(&stat, MPI_BYTE, &taille1);
                                //printf("Get count taille recieved : %d\n" , taille1);

                                to_recv = (char *)malloc(taille1 * sizeof(char)); // free it!
                                MPI_Recv(to_recv , taille1 , MPI_CHAR , stat.MPI_SOURCE , MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                                //printf("value to GET in LOCAL KVS : %s \n" , to_recv);
                                returned_value = NULL; // will containe the value corresponding to the key received by MPI_Recv
                                taille_out = 0; // will contain the size of the value
                                // &returned_value instead of returned_value !! 
                                check_get = local_kvs_get(to_recv, &returned_value, &taille_out, local_kvs_root);

                                if(check_get == 0){
					MPI_Send(returned_value, taille_out + 1, MPI_CHAR, stat.MPI_SOURCE, CHECK_GET_TAG, MPI_COMM_WORLD) != 0 ;
                                        //puts("Found !");  
                                }
                                else {
                                        //puts("KEY NOT FOUND IN LOCAL KVS !!");
                                        MPI_Send(NULL, 0, MPI_BYTE, 1, TAG_NOT_FOUND, MPI_COMM_WORLD) ;
                                }

                                break;

                        case (TAG_FINISH) :
                                //assert(stat.MPI_TAG == TAG_FINISH);
                                MPI_Recv(NULL, 0, MPI_BYTE, stat.MPI_SOURCE, TAG_FINISH, MPI_COMM_WORLD ,MPI_STATUS_IGNORE);
                                //puts("Task finished !!");
                                finished = 1 ;
                               
		}
	
	}while(!finished);	


} /*  END of  MPI_Server_process_requests() */


