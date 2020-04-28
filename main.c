#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "kvs.h"
#include "murmur3.h"
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <limits.h>
#include <kvs_mpi.h>

#define TESTS 100

int main(int argc , char ** argv){
	
	if ( argc > 2 || argc == 0 ){
	    printf("%s" , "Enter the size of the Table");
    	return 0 ;
  	}
 
	const int N = atoi(argv[1]) ;

	MPI_Init(&argc , &argv);
	
	int rank , P;  
	
	MPI_Comm_rank(MPI_COMM_WORLD , &rank);
	MPI_Comm_size(MPI_COMM_WORLD , &P);
	
	if (P <= 1){
		printf("%s\n" , "Requires at least two processes");
		MPI_Finalize();
		return 0;
	}	

	//MPI_Datatype 	MPI_Key_vsize ;
	//build_data_type(MPI_Key_vsize);

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



	if (rank == KVS_SERVER){
		Initialise_kvs(N);
		srand(time(0));

		double t_start, t_end;
		
		t_start = MPI_Wtime();
		
		process_new_request(rank);
		
		t_end	= MPI_Wtime();
		
		long long int nbr_test_global = NUMBER_OF_TESTS_PER_PROCESS * TESTS * P ;
		printf("Numbers of TESTS_PER_PROCESS = %d\nNumber of TESTS = %d\nTotal number of Tests global = %d*%d%d = %lld\n", NUMBER_OF_TESTS_PER_PROCESS, TESTS, NUMBER_OF_TESTS_PER_PROCESS, TESTS, P, nbr_test_global);
		
		printf("Time elapsed	: %f\n", t_end - t_start);
		return 0 ;
	}

	else {	

		for(int i = 0; i < TESTS; i++){
			run_data();  // insert random keys-values in kvs	
			lookups();   // lookup for random keys
		}
		//puts("test finished");
		MPI_Send(NULL, 0, MPI_BYTE, KVS_SERVER, TAG_FINISH, MPI_COMM_WORLD) ;


	}

	MPI_Finalize();
	return 0;
}
