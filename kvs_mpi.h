#ifndef _KVS_MPI_H
#define _KVS_MPI_H

#include <stdio.h>
#include <stddef.h>

#define KVS_SERVER 0

#define SET_TAG 1000
#define GET_TAG 2000

#define CHECK_SET_TAG 1111
#define CHECK_GET_TAG 2222

#define TAG_NOT_FOUND 88
#define TAG_FINISH    99

#define KEY_MAX 100 

#define  NUMBER_OF_PAIRS 1000
#define	 NUMBER_OF_LOOKUPS 1000
#define  NUMBER_OF_TESTS_PER_PROCESS   1000

#if	UINTPTR_MAX == 0xffffffffffffffffULL
#define	BUILD_64 1
#endif

typedef struct key_Vsize_s{
	size_t size_value ;
	char key[KEY_MAX] ;
} key_Vsize_t;

typedef char* Value_Buff_t  ; 



void random_key_value(key_Vsize_t* kv, Value_Buff_t *value) ;

void random_key(char** key);

void process_new_request(int rank);
void run_data();
void lookups();
#endif
