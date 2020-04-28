
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
#include "kvs_mpi.h"

void random_key_value(key_Vsize_t* kv, Value_Buff_t *value){	
	int a = rand(), 
	    b = rand();
	sprintf(kv->key	 , "%d" , a);	
	*value = (char *)malloc(sizeof(char));
	sprintf(*value	 , "%d" , b);	
	kv->size_value = strlen(*value) + 1 ;
	/*--Test --
	strcpy(kv->key, "abc");
	*value = (Value_Buff_t)malloc(6 * sizeof(char)); //	
	*value = "VVVVVVV" ;
	*/	
}

void random_key(char** key){
	int a = rand()  ;
	sprintf(*key, "%d", a);
	
	// --Test
	//strcpy(*key, "abc");
}

/*void build_data_type(MPI_Datatype *MPI_Key_vsize){

	int 		blocklens[2] = {1, KEY_MAX};
	
	MPI_Aint 	displacements[2] ;

	MPI_Aint 	offsets[2];
	
       	// check the compiler first : size_t might be unsigned long or unsigned long long !

	MPI_Datatype	typelist[2] = { MPI_UNSIGNED_LONG_LONG, MPI_CHAR};
	

	displacements[0] = offsetof(struct key_Vsize_s, size_value) ;
        displacements[1] = offsetof(struct key_Vsize_s, key) ;	
	
	MPI_Type_create_struct(2, blocklens, displacements, typelist, &(*MPI_Key_vsize));
	MPI_Type_commit(&(*MPI_Key_vsize));	
}*/

void process_new_request(int rank){
	if(rank != 0){
		perror("process_new_request");
		exit(1);
	}

	key_Vsize_t 	to_receive1; // key , size_value
	Value_Buff_t 	to_receive2;

	char* returned_value;
	char* to_recv;
	MPI_Status stat;
	int taille1 ;
	size_t taille_out ;
	int arrived = 0;
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


	//MPI_Message mes;
	
	int finished = 0 ; // changed only in default switch statement 
	do{

		/* change this to MPI_Mprobe ! */
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
	
	//	MPI_Mprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &mes, &stat);
	
		int check_put, check_get ;
				
		switch (stat.MPI_TAG){
			
			case (SET_TAG) :
				// Receiving message if MPI_Probe correspond to kvs_set
				// Use MPI_Mrecv instead of MPI_Recv !
					
				MPI_Recv(&to_receive1 , 1, MPI_Key_vsize , stat.MPI_SOURCE , SET_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

			//	MPI_Mrecv(&to_receive1 , 1, MPI_Key_vsize , &mes, MPI_STATUS_IGNORE);
			
				to_receive2 = (char*)malloc( to_receive1.size_value * sizeof(char));	
				MPI_Recv(to_receive2, to_receive1.size_value, MPI_CHAR, stat.MPI_SOURCE, SET_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

			//	MPI_Mrecv(to_receive2, to_receive1.size_value, MPI_CHAR, &mes, MPI_STATUS_IGNORE);

				//puts("IN : SET___________TAG");	

				//printf("data : %s\t%ld\t%s\n", to_receive1.key, to_receive1.size_value, to_receive2);	
				check_put = kvs_put(to_receive1.key, to_receive2, to_receive1.size_value);		
			
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
				//printf("value to GET in KVS : %s \n" , to_recv);
				returned_value = NULL; // will containe the value corresponding to the key received by MPI_Recv
				taille_out = 0; // will contain the size of the value
				// &returned_value instead of returned_value !!	
				check_get = kvs_get(to_recv, &returned_value, &taille_out);
			        
				if(check_get == 0){
					//puts("Found");       
					MPI_Send(returned_value, taille_out + 1, MPI_CHAR, stat.MPI_SOURCE, CHECK_GET_TAG, MPI_COMM_WORLD) != 0 ;
						//puts("Sent");
					
				}
				else {
					//puts("KEY NOT FOUND IN KVS !!");
					MPI_Send(NULL, 0, MPI_BYTE, 1, TAG_NOT_FOUND, MPI_COMM_WORLD) ;	
				}
				
				break;	

			case (TAG_FINISH) :
				assert(stat.MPI_TAG == TAG_FINISH);
				MPI_Status st ;
				MPI_Recv(NULL, 0, MPI_BYTE, stat.MPI_SOURCE, TAG_FINISH, MPI_COMM_WORLD ,&st);
				//puts("Task finished !!");
				finished = 1 ;
				//MPI_Finalize();
				//return ;	
			}
	
		}while(!finished);

}  /* End of recv_new_request() */


void run_data(){

	key_Vsize_t *to_send_1 = (key_Vsize_t *)malloc(1 * sizeof(key_Vsize_t));
	Value_Buff_t	Value  = NULL; 
		
	char* get_key = (char *)malloc( KEY_MAX * sizeof(char) );
	char* get_value;
	int   put_checked;
	//int   get_checked;
	int   taille;
	char* value = NULL;
	MPI_Status stat2 ;
//	MPI_Datatype *MPI_Key_vsize;

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


	//build_data_type(MPI_Key_vsize);

	for(int i = 0 ; i < NUMBER_OF_PAIRS ; ++i){
		random_key_value(to_send_1 , &Value);
		
		//printf("data to send : \nkey = %s\t value = %s\t size_value = %ld\n", to_send_1->key, Value, to_send_1->size_value);
			
		//printf("sizeof(to_send) =  %ld\n", sizeof(to_send));
		//puts("debug");	

		MPI_Send(to_send_1, 1, MPI_Key_vsize , KVS_SERVER, SET_TAG, MPI_COMM_WORLD);
		//puts("1st sent ");
		MPI_Send(Value, to_send_1->size_value, MPI_CHAR, KVS_SERVER, SET_TAG, MPI_COMM_WORLD);
		//puts("Test Set");
		MPI_Recv(&put_checked, 1, MPI_INT, KVS_SERVER, CHECK_SET_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		//puts("SUCCESS set in kvs \n");
	
		
	
	}

} /* End of run_tests_requests*/

void lookups(){
		char* get_key = (char *)malloc( KEY_MAX * sizeof(char) );
		char* get_value;
		//int   put_checked;
		//int   get_checked;
		int   taille;
		MPI_Status stat2 ;

		for(int i = 0; i< NUMBER_OF_LOOKUPS; ++i){

			random_key(&get_key);
			//printf("Random key : %s , %ld\n", get_key, strlen(get_key) * sizeof(char));
			
			MPI_Send(get_key, strlen(get_key) + 1 , MPI_CHAR, KVS_SERVER, GET_TAG, MPI_COMM_WORLD);
			//puts("Avant Probe");	
			
			MPI_Probe(KVS_SERVER, MPI_ANY_TAG, MPI_COMM_WORLD, &stat2) ;			
			//puts("Après Probe");

			if (stat2.MPI_TAG == TAG_NOT_FOUND){
				MPI_Status stt;
				//puts("Je suis en Erreur 88");
				MPI_Recv(NULL, 0, MPI_BYTE, KVS_SERVER, TAG_NOT_FOUND, MPI_COMM_WORLD ,&stt);
	
				//puts("KEY NOT FOUND IN KVS FROM PROCESSUS 1");
			}

			if (stat2.MPI_TAG == CHECK_GET_TAG){
				//puts("-------------");	
				MPI_Get_count(&stat2, MPI_BYTE, &taille);
				get_value = (char *)malloc(taille * sizeof(char));
				MPI_Recv(get_value, taille, MPI_CHAR, KVS_SERVER, CHECK_GET_TAG, MPI_COMM_WORLD, &stat2);
				//printf("MPI Get answer %s\n", get_value); fflush(stdout);
				free(get_value);	
			}
		}
}
