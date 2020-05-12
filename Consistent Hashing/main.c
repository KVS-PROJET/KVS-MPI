#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "murmur3.h"
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <limits.h>
#include <mpi_const_hash.h>
#include <Consistent_Hashing.h>
#include <local_kvs.h>
#include <stdint.h>

#define TESTS 100
#define SEED 42

// BST initialisé à NULL, contienera tous les serveurs après appel à la fonction : Initialize_ring();
node_t * Server_ring_g = NULL;


// On teste l'insertion dans un premier temps
void insert_random_data(int rank , int limit_hash_space){
	int a = rand() - rank , b = rand();
	char key[100];
	char value[100];
	sprintf(key, "%d", a);
	//char* key = "abc";
	sprintf(value, "%d", b);
	size_t in_data = strlen(value);	
	MPI_Client_set_data(key, value, in_data, Server_ring_g, limit_hash_space);
}

void lookup_for_random_data(int limit_hash_space){
	int a = rand();
	char key[100];
	sprintf(key, "%d", a);
	char* value = NULL;
	size_t out_data = 0 ;
	MPI_Client_get_data(key, &value, &out_data, Server_ring_g, limit_hash_space);	
}



int main(int argc , char ** argv){

	MPI_Init(&argc , &argv);
	
	int rank , nproc;  
	
	MPI_Comm_rank(MPI_COMM_WORLD , &rank);
	MPI_Comm_size(MPI_COMM_WORLD , &nproc);
		
	if (nproc <= 1){
		printf("%s\n" , "Requires at least two processes");
		MPI_Finalize();
		return 0;
	}	
	srand(time(0));

	MPI_Datatype MPI_Key_vsize; 
	build_data_type(&MPI_Key_vsize);
	

	//-------------------------------------------------------------------------------------
	int nbr_servers, nbr_clients ;
	long int hash_space_limit ;
	long int number_of_tests ;
	int chunk, part ;
	long int req_per_client;
	if(argc != 4){
		perror(" Enter exactly 4 correct arguments");
		return 1;
	}
	else{
		nbr_servers 		=	atoi(argv[1]);
		hash_space_limit	=	atoi(argv[2]);
		number_of_tests		=	atoi(argv[3]);
		
		if(nbr_servers > nproc / 2  ||  hash_space_limit <  nproc  || number_of_tests <= 0){
			perror("Enter correct arguments !");
			return 1;
		}
		
		chunk	= hash_space_limit / nbr_servers;
		part	= nproc / nbr_servers;
		
		nbr_clients 	= nproc - nbr_servers;
		req_per_client = number_of_tests / nbr_clients ;
	}
				
	// Initialize BST of servers ---------------------------------------------------------	
	Initialize_ring(&Server_ring_g , nbr_servers , nproc);

	double elapsed_time = 0. , elapsed_time_g = 0.;
	long int nbr_opr = 0 ;

	if( rank == 0){
		printf("chunk = %d\n", chunk);
		printf("part = %d \n", part);
		printf("nbr req / Client = %ld\n", req_per_client);
	}
		
	local_kvs_t* local_kvs_root = NULL;	
	
	if (rank % part == 0){

		MPI_Server_process_requests(&local_kvs_root, &elapsed_time, &nbr_opr);
	
		//printf("Number of operation for rank %d = %ld \t elapsed time = %f\n", rank, nbr_opr, elapsed_time); fflush(stdout);
	}
	else {	
		for(long int i = 0; i <  req_per_client ; i++)
			insert_random_data(rank, hash_space_limit);  	
		
		for(long int i = 0; i <  req_per_client ; i++)
			lookup_for_random_data(hash_space_limit);	
	}

	MPI_Reduce(&elapsed_time, &elapsed_time_g, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);


	int target_client = -1 ;	
	if (rank == 0)	
		printf("Global time elapsed %f\n", elapsed_time_g);

	if((rank % part != 0)  && ( rank != target_client) )	
		MPI_Finish_task(nbr_servers, nproc, target_client, part);
	else
		MPI_Server_process_requests(&local_kvs_root, &elapsed_time, &nbr_opr);
	//--------------------------------------------------------------------------------------

#ifdef _DYNAMIC_ADD_REMOVE_SERVER_

	/* Add a server randomly */
	target_client = INT_MAX % nproc ;
	//target_client = 6;
	int new_server ;
	int msg_bcast[2] ;
	if(rank == 0){
		puts("Server to Add :");
		printf("target_client = %d\n", target_client);	
	}
	if ( target_client % part != 0 ){ // not already a server
		if( rank == target_client)
			new_server = target_client ;

		msg_bcast[0] = 1 ; // 1 for adding a server, -1 for removing one
		msg_bcast[1] = new_server;			
	}
	else{
		puts("Any server added, recompile!");
	}

	// Bcast the msg to all process to add the new added server
	MPI_Bcast(msg_bcast, 2, MPI_INT, target_client, MPI_COMM_WORLD);

	// All processes receives the broadcasted msg, refresh the ring	and move the eventual data
	MPI_Refresh_ring(&Server_ring_g, msg_bcast, chunk, &nbr_servers, part, &local_kvs_root, hash_space_limit);
	if (rank == 0)
		printf("process %d added to servers\n" ,target_client);
	
	//Server_BST_display(Server_ring_g);

	// We process random data to our new set of servers
	if (rank % part == 0 || rank == target_client){	
		MPI_Server_process_requests(&local_kvs_root, &elapsed_time, &nbr_opr);
	}
	else{
		for(long int i = 0; i < 1; i++)
			insert_random_data(rank, hash_space_limit);
	}


	if(rank % part != 0 && rank != target_client)	
		MPI_Finish_task(nbr_servers, nproc, target_client, part);
	else
		MPI_Server_process_requests(&local_kvs_root, &elapsed_time, &nbr_opr);
	
#endif		
	
	MPI_Finalize();
	return 0;
}
