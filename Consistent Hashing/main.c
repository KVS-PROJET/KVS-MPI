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

		
// BST initialisé à NULL, contienera tous les serveurs après appel à la fonction : Initialize_ring();
node_t * Server_ring_g = NULL;


// On teste l'insertion dans un premier temps
void run_data(){
	int a = rand(), b = rand();
	char key[100];
	char value[100];
	sprintf(key, "%d", a);
	sprintf(value, "%d", b);
	size_t in_data = strlen(value);	

	MPI_Client_set_data(key, value, in_data, Server_ring_g);
}

// int glob = 6; juste pour tester var global..

int main(int argc , char ** argv){
	
	
	MPI_Init(&argc , &argv);
	
	int rank , P;  
	
	MPI_Comm_rank(MPI_COMM_WORLD , &rank);
	MPI_Comm_size(MPI_COMM_WORLD , &P);
	
	if (P <= 1){
		printf("%s\n" , "Requires at least two processes");
		MPI_Finalize();
		return 0;
	}	

	srand(time(0));

	//MPI_Datatype 	MPI_Key_vsize ;
	//build_data_type(MPI_Key_vsize);-------------------------------------------
	
	MPI_Datatype MPI_Key_vsize; 
	
	int 		blocklens[2] = {1, KEY_MAX};
	
	MPI_Aint 	displacements[2] ;

	MPI_Aint 	offsets[2];
	
       	// check the compiler first : size_t might be unsigned long or unsigned long long !

	MPI_Datatype	typelist[2] = { MPI_UNSIGNED_LONG_LONG, MPI_CHAR};
	

	displacements[0] = offsetof(struct key_Vsize_s, size_value) ;
        displacements[1] = offsetof(struct key_Vsize_s, key) ;	
	
	MPI_Type_create_struct(2, blocklens, displacements, typelist, &MPI_Key_vsize);
	MPI_Type_commit(&MPI_Key_vsize);
	//--------------------------------------------------------------------------------
	
	//printf("glob = %d\n", glob);
	
	// Initialize BST of servers -------------------------------------------------- 
	Initialize_ring(&Server_ring_g   /*, &glob*/);
	printf("server_ring_g = %p\n", Server_ring_g);
	//printf("glob = %d\n", glob);
	//----------------------------------------------------------------------------
	
	if (rank %2 == 0){
		// Initialize local kvs in every server
		// local_kvs_root is also a BST that represents the local kvs in this server.
		local_kvs_t* local_kvs_root = NULL;

		//printf("server_ring_g = %p\n", Server_ring_g);	

		MPI_Initilize_local_kvs(local_kvs_root);
		
//MPI_Initilize_server_ring(local_kvs_root/*, rank, &Server_ring_g*/); // voir mpi_const_hash.c
		
printf("server_ring_g initialized = %p", Server_ring_g);		
		//printf("rank = %d" , rank);
		

		double t_start, t_end;
		
		t_start = MPI_Wtime();
		
		/*chaque serveur traite les requetes correspond à son local kvs
		 *en se basant sur le principe de consistent hashing où les serveurs sont regroupé 
		 *dans une BST structure : Server_ring_g (par leur valeur hash obtenu par MurmurHash)
		 *Les valeurs hash des clés et les valeurs hash des serveurs sont regroupés dans une structure d'anneau abstraite
		 *de telle façon à ce que :
		 **Si par exemple deux serveurs de valeur hashs i et j respictivement (tel que le serveur j vient juste après le serveur i dans l'anneau : i.e le serveur j est successeur de i dans la structure BST des serveurs)
		 **Alors le serveur j prend en charge tous les clés
		  *dont la valeurs hash est compris entre i (inclus) et j .
		*De cette façon la charge sera répartie sur les serveurs	
		*/
		MPI_Server_process_requests(local_kvs_root);

		t_end	= MPI_Wtime();
		
		//long long int nbr_test_global = NUMBER_OF_TESTS_PER_PROCESS * TESTS * P ;
		//printf("Numbers of TESTS_PER_PROCESS = %d\nNumber of TESTS = %d\nTotal number of Tests global = %d*%d%d = %lld\n", NUMBER_OF_TESTS_PER_PROCESS, TESTS, NUMBER_OF_TESTS_PER_PROCESS, TESTS, P, nbr_test_global);
		
		//printf("Time elapsed	: %f\n", t_end - t_start);
		//return 0 ;
	
	}

	else {	

		for(int i = 0; i < 1/*TESTS*/; i++){
			run_data();  	
		}
	

	}	
	MPI_Finish_task(P);

	MPI_Finalize();
	return 0;
}
