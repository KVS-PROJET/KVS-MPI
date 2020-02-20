#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "kvs.h"
#include "murmur3.h"
#include <sys/time.h>

#define KVS_SERVER 0

#define SET_TAG 1000
#define GET_TAG 2000

#define CHECK_SET_TAG 1111
#define CHECK_GET_TAG 2222

#define KEY_MAX 10 ;

#define  NUMBER_OF_TESTS 10;

typedef struct {
	size_t size_value;
	char * key;
	char * value;
} Key_Value_Struct;

void random_key_value(Key_Value_Struct* kv){	
	srand(time(0));
	int a = rand(), b = rand();
	sprintf(kv->key   , "%d" , a);
	sprintf(kv->value , "%d" , b);
	kv->size_value = strlen(kv->value);
}
void random_key(char* key){
	srand(time(0));
	int a = rand();
	sprintf(key, "%d", a);
}

int main(int argc , char ** argv){
	
	if ( argc > 2 || argc == 0 ){
	    printf("%s" , "Enter the size of the Table");
    	return 0 ;
  	}
 
	const int N = atoi(argv[1]) ;

	MPI_Init(&argc , &argv);
	
	int rank , int P;  
	
	MPI_Comm_rank(MPI_COMM_WORLD , &rank);
	MPI_Comm_size(MPI_COMM_WORLD , &P);
	
	if (P <= 1){
		printf("%s\n" , "Requires at least two processes");
		MPI_Finalize();
		return 0;
	}
		
	if (rank == KVS_SERVER){
		Initialise_kvs(N);
		Key_Value_Struct to_receive;
		char* returned_value;
		MPI_STATUS stat;
		int arrived = 0;
		
		// replace this while loop with do-while-loop and add MPI_Probe to checking for new income messages
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
		
		while (stat.MPI_SOURCE > 0){
						
			MPI_Recv(&to_receive , sizeof(to_receive) , MPI_BYTE , MPI_ANY_SOURCE , MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
			//Check whether the message is about setting a nex key-value in the table or looking for a value corresponding to a given key : 
			switch (stat.MPI_TAG){
				case SET_TAG :
					int check_put = kvs_put(to_receive.key, to_receive.value, &to_receive.size_value);
				
					MPI_Send(&check_put, 1, MPI_INT, stat.MPI_SOURCE, CHECK_SET_TAG, MPI_COMM_WORLD);
					break;
				case GET_TAG :
					int check_get = kvs_get(to_receive.key, &to_receive.value, &to_receive.size_value);
					returned_value = malloc(to_receive.size_value * sizeof(char));
					MPI_Send(&returned_value, 1, MPI_BYTE, stat.MPI_SOURCE, CHECK_GET_TAG, MPI_COMM_WORLD);
					break;	
			}

			MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
		}
	}
	else {	
		Key_Value_Struct to_send;
		char* get_key;
		char* get_value;
		int   put_checked;
		//int   get_checked;
		int   taille;
		char* value = NULL;
		MPI_STATUS stat2 ;
		
		for(int i = 0 ; i < NUMBER_OF_TESTS ; ++i){
			random_key_value(&tosend);

			MPI_Send(&to_send, sizeof(to_send), MPI_BYTE, KVS_SERVER, SET_TAG, MPI_COMM_WORLD);
			MPI_Recv(&put_checked, 1, MPI_INT, KVS_SERVER, CHECK_SET_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			random_key(&get_key);
			MPI_Send(&get_key, 1, MPI_BYTE, KVS_SERVER, GET_TAG, MPI_COMM_WORLD);
			
			MPI_Probe(KVS_SERVER, CHECK_GET_TAG, MPI_COMM_WORLD, &stat2);

			if (stat2.MPI_SOURCE == 0){
				MPI_Get_Count(&stat2, MPI_BYTE, &taille);
				get_value = malloc(taille * sizeof(char*));
				MPI_Recv(get_value, taille, MPI_BYTE, stat2.MPI_SOURCE, stat2.MPI_TAG, MPI_COMM_WORLD, &stat2);
				prtinf("Processe number %d receive %s\n", rank, get_value); fflush();
				
			}
		}	
	
	}
	MPI_Finalize();
	return 0;
}
