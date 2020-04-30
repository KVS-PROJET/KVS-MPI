#include <stdio.h>
#include <stdlib.h>
#include "murmur3.h"
#include <string.h>
#include <local_kvs.h>
#include <stddef.h>
#include <stdint.h>

//#define _TEST_LOCAL_KVS_

#define SEED 42  // used by murmurHash



// allocate binary search structure to store key-value locally in servers
local_kvs_t* make_local_kvs(uint32_t key_hash, char* key, char* value){
        local_kvs_t* local_kvs_entry  = (local_kvs_t*)malloc(sizeof(local_kvs_t));
        
	local_kvs_entry -> key_hash  	= key_hash;
        local_kvs_entry -> key 		= key;
	local_kvs_entry -> value     	= value;
	
	local_kvs_entry ->left    = NULL;
        local_kvs_entry ->right   = NULL;
        return local_kvs_entry ;
}

/* add an entry to the local kvs*/
local_kvs_t* add_entry(local_kvs_t* root, uint32_t key_hash, char* key, char* value){ 
        if( root == NULL){              /*first node*/
                        return make_local_kvs(key_hash, key, value) ;
        }
        else if (key_hash > root->key_hash){
                        root->right = add_entry(root->right , key_hash, key, value);
        }
        else if (key_hash < root->key_hash){
                        root->left = add_entry(root->left , key_hash, key, value);
        }
        return root ;
}



/* find the smallest key in the local kvs */
int find_low_local_key(local_kvs_t* root, local_kvs_t** target_entry){
        if (root == NULL){
                *target_entry = root ;
                return 0 ;
        }
        else if (root->left != NULL)
                return find_low_local_key(root->left, target_entry);
	else {
		*target_entry = root;
		return 0;
		//return find_low_local_key(root->, target_entry);
	}
	return 1;
}


/* remove a key from a local kvs*/
local_kvs_t* remove_local_key(local_kvs_t* root, uint32_t to_remove){
        if (root == NULL)
                return root;
        else if(to_remove > root->key_hash)
                root->right =  remove_local_key(root->right, to_remove);
        else if(to_remove < root->key_hash)
                root->left  =  remove_local_key(root->left , to_remove);
        else{
                if(root->right == NULL && root->left == NULL){
                        free(root);
			return NULL;
		}
		else if(root->right == NULL || root->left == NULL){   	// change it to XOR !
                        local_kvs_t *tmp = (local_kvs_t*)malloc(sizeof(local_kvs_t));
                        if (root->right == NULL){
                                tmp = root->left;
				root->key_hash 	=  tmp->key_hash ;
				root->key	=  tmp->key;
				root->value	=  tmp->value;
                           	//root->left = remove_local_key(root->left, tmp->key_hash);

			   	//free(tmp);
                                return tmp;
                        }
                        else{
                        	tmp = root->right;
				root->key_hash 	=  tmp->key_hash ;
				root->key	=  tmp->key;
				root->value	=  tmp->value;
                               	//remove_local_key(root->right, tmp->key_hash);

				//free(tmp);
                                return tmp;
                        }
                }
                else{
                        //local_kvs_t* low_key 	= (local_kvs_t*)malloc(sizeof(local_kvs_t));
                        local_kvs_t* tmp_ 	= (local_kvs_t*)malloc(sizeof(local_kvs_t));
			find_low_local_key(root->right, &tmp_);
			//tmp_	=	low_key ;
                        root->key_hash 	= tmp_->key_hash;
			root->key	= tmp_->key;
			root->value	= tmp_->value;
			root->right = remove_local_key(root->right, tmp_->key_hash);
			
			//return ;
                        //root->right = remove_local_key(root->right, low_key->key_hash);
                }
        }
        return root ;
}


/*searching for a key in a local kvs*/
int search_key(local_kvs_t* root, uint32_t key_, local_kvs_t ** target_entry){
        if (root == NULL )
		return 0;
	else if (root->key_hash == key_){
                *target_entry = root;
                return 1;	// key found
        }
        else if (key_ > root->key_hash)
                search_key(root->right , key_, target_entry);
        else if (key_ < root->key_hash)
                search_key(root->left  , key_, target_entry);
	else 
       		return 0 ; 		// key not found
}

/* TO DO */

int MPI_Initilize_local_kvs(local_kvs_t* root){
	if (root != NULL)
        	root = NULL;
        return 0;
}

int local_kvs_put(char* local_key, char* local_value, size_t size_data, local_kvs_t** local_kvs_root){
       	local_kvs_t* target_entry = (local_kvs_t*)malloc(sizeof(local_kvs_t));
	uint32_t hash_key = 0;
	int len = strlen(local_key);
	uint32_t seed = SEED ;
	MurmurHash3_x86_32(local_key, len, seed, &hash_key); //
	
	int found =  search_key(*local_kvs_root, hash_key, &target_entry);
	// search_key take just O(log N) to find the key, because of the structure of binary tree that we adopt to store key-values .
	if (found == 0){  // if not found, we add it to the local kvs
		*local_kvs_root = add_entry(*local_kvs_root, hash_key, local_key, local_value);
		return 0 ;
	}
	else{		 //TO DO  if its found update the value !
		target_entry -> key	= local_key;
		target_entry -> value	= local_value;
		return 0 ;	
	}
	free(target_entry); //
	return 1 ;
}

int local_kvs_get(char* local_key, char** local_value, size_t* size_data, local_kvs_t* local_kvs_root){
	local_kvs_t* get_entry = (local_kvs_t*)malloc(sizeof(local_kvs_t));
	uint32_t hash_key = 0;
	int len = strlen(local_key);
	uint32_t seed = SEED ;
	MurmurHash3_x86_32(local_key, len, seed, &hash_key); 	
	
	int found =  search_key(local_kvs_root, hash_key, &get_entry);
	if(found == 0){  // Not found !		
		return 1;
	}
	else{ 	// key found
		*local_value 	=  get_entry->value ;
		*size_data	=  strlen(*local_value);
		return 0;
	}
	free(get_entry); //

}

/* inorder traversal */
void inorder(local_kvs_t* root){
	if (root == NULL)
		return ;
	else {
		inorder(root->left);
		printf("key_hash = %ud\t" ,root->key_hash);
		printf("key      = %s\t" ,root->key);
		printf("value    = %s\n" ,root->value);
		inorder(root->right);
	}
}

#ifdef _TEST_LOCAL_KVS_

/* Test the binary search structure*/
int main(int argc, char** argv){
	local_kvs_t* root = NULL;
	MPI_Initilize_local_kvs(&root);
	local_kvs_put("ab", "xyz", 3, &root);	
	local_kvs_put("rr", "ppp", 3, &root);	
	local_kvs_put("tt", "eeee", 4, &root);	
	local_kvs_put("gg", "jjjj", 4, &root);	
	local_kvs_put("ee", "rrrr", 4, &root);	
	local_kvs_put("zz", "uuuu", 4, &root);	
	local_kvs_put("aa", "yyyyyy", 6, &root);	


	char *val = NULL;
	size_t t = 0;
	int ff = local_kvs_get("ab", &val, &t, root);
	printf("ff = %d \t value = %s\tsize = %lu\n", ff, val, t);
	ff = local_kvs_get("gg", &val, &t, root);
	printf("ff = %d \t value = %s\tsize = %lu\n", ff, val, t);

	
/*
	root = make_local_kvs(3,"abc", "xyz");
	root = add_entry(root, 10, "de", "tttt");
	root = add_entry(root, 21, "ee", "rrrr");
	root = add_entry(root, 1, "ddd", "ssss");
	root = add_entry(root, 2, "zz", "jjjj");
	root = add_entry(root, 8, "mm", "pppp");
	root = add_entry(root, 15, "kk", "kkkk");
	root = add_entry(root, 0, "aa", "zrzr");
	root = add_entry(root, 4, "bb", "rbrb");
	root = add_entry(root, 16, "ss", "kfg");
*/
	inorder(root);
	//local_kvs_t* target = (local_kvs_t*)malloc(sizeof(local_kvs_t));
/*	Test Removing entry	*/
	//target = remove_local_key(root, 15);
	//printf("remove target->key_hash = %d\n", target->key_hash);
	//inorder(root);

/*	puts("Test Searching ------------");
	int fd = search_key(root, 0, &target);
	printf("Found %d\n key : %d, %s\n", fd, target->key_hash, target->key);
	printf("&root = %p\t&target = %p\n", root, target);
*/

/*	puts("Test low local-------------");
	local_kvs_t* target_ = (local_kvs_t*)malloc(sizeof(local_kvs_t));
	int found = find_low_local_key(root, &target_);
	printf("%d\n", found);
	printf("&root = %p\t&target_ = %p\t value = %s\n", root, target_, target_->value);
*/	
	return 0;
}
#endif

