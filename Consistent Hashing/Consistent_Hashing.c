#include <Consistent_Hashing.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include "murmur3.h"
#include <string.h>
//#include <mpi_const_hash.h>

/* GLobal BST structure that represents the ring of servers . 
* Initialized in mpi_const_hash.c  : 
* Voir MPI_Initilize_server_ring(local_kvs_t** local_kvs_root, int rank)
*/

#define SEED 42 


/* node_ID représente la valeur hash du serveur obtenu par la fonction de hashage murmurHash*/
node_t* make_node(int rank, uint32_t new_id){
	node_t* new_node  	= (node_t*)malloc(sizeof(node_t));
	
	new_node->node_ID 	= 	new_id;  
	new_node->true_rank	=	rank ;

	new_node->left	  = NULL; 
	new_node->right	  = NULL;
	return new_node ;
}


/* ajouter un serveur au structure BST*/
node_t* add_server(node_t* root, int rank , uint32_t new_id){  /* */
	if( root == NULL){ 		/*first node*/
			return make_node(rank , new_id) ;
	}
	else if (new_id > root->node_ID){ /*add server to the right side*/
			root->right = add_server(root->right , rank, new_id);
	}
	else if (new_id < root->node_ID){ /*add server to the left side*/
			root->left = add_server(root->left , rank, new_id);
	}
	return root ;
}

/* chercher le serveur de plus petit id */
int find_low_id(node_t* root, node_t** target_node){
	if (root == NULL){
		*target_node = root ;
		return 0 ;
	}
	else if (root->left != NULL)
		return find_low_id(root->left, target_node);
	else {
		*target_node = root;
		return 0;
	}
	return 1;
}

/* remove a server*/
node_t* remove_server(node_t* root, uint32_t to_remove){
	if (root == NULL)
		return root;
	else if(to_remove > root->node_ID)
		root->right = remove_server(root->right, to_remove);
	else if(to_remove < root->node_ID)
		root->left  = remove_server(root->left , to_remove);
	else{
		if(root->right == NULL && root->left == NULL){
			free(root);
			return NULL;
		}
		else if(root->right == NULL || root->left == NULL){
			node_t *tmp = (node_t*)malloc(sizeof(node_t));
			if (root->right == NULL){
				tmp = root->left;
				root->node_ID	=  tmp->node_ID;
				root->true_rank	=  tmp->true_rank ;
				//free(root);
				return tmp;
			}
			else{
				tmp = root->right;
				root->node_ID	=  tmp->node_ID;
				root->true_rank	=  tmp->true_rank ;
				//free(root);
				return tmp;
			}
		}
		else{
			node_t* low_node = (node_t*)malloc(sizeof(node_t)); 
			find_low_id(root->right, &low_node);	
			root->node_ID	 = low_node -> node_ID;
			root->true_rank	 = low_node -> true_rank ;
			root->right = remove_server(root->right, low_node->node_ID);
		}
	}
	return root ;
}



int search_server(node_t* root, int server_rank, node_t ** target_node){
	if (root == NULL )
		return 0;
	else if(root->true_rank == server_rank){
		*target_node = root;
		return 1 ;
	}
	else if (server_rank > root->true_rank)
		search_server(root->right , server_rank, target_node);
	else if (server_rank < root->true_rank)
		search_server(root->left  , server_rank, target_node);
	else
		return 0 ;
}


/*
trouver le successeur d'un serveur : 
 * utile lors de l'ajout/suppression d'un serveur:
 *  pour pouvoir copier les éléments du successeur correspondants au nouveau serveur
 *  ou de copier les élément de serveur supprimé vers le successeur
*/
int find_successor(node_t *root, node_t* root_, uint32_t id, int *rank_successor, int *id_successor){
	if (root == NULL) {
		*rank_successor = -1 ;
		*id_successor	= -1 ;
		return 1 ;
	}
	else if (id == root->node_ID){
		*rank_successor = root->true_rank ;
		*id_successor	= root->node_ID ;
		return 0;
	}
	else if (root->left == NULL || root->right == NULL){
		if ( root->right != NULL && id > root->node_ID ){			
			node_t* tmp = root->right ;
			if( id <= tmp->node_ID){
				*rank_successor = tmp->true_rank ;
				*id_successor	= tmp->node_ID ;
				return 0 ;
			}
			else 
				find_successor(tmp, root_, id, rank_successor, id_successor);	
		}
		else if ( root->left != NULL && id < root->node_ID ){
			node_t* tmp = root->left ;
			if ( id <= tmp->node_ID){
				*rank_successor = tmp->true_rank ;
				*id_successor	= tmp->node_ID ;
				return 0;
			}
			else
				find_successor(tmp, root_, id, rank_successor, id_successor);
			}
		else{	
			node_t *target_node = NULL;
	 		find_low_id( root_ , &target_node);
			*rank_successor	= root_ -> true_rank ;
			*id_successor	= root_ -> node_ID ;
			return 0;
		}	
	}
	else if (id > root->node_ID) {
		node_t* tmp = root->right ;
		if( id <= tmp->node_ID){
			*rank_successor = tmp->true_rank ;
			*id_successor	= tmp->node_ID ;
			return 0 ;
		}
		else 
			find_successor(tmp, root_, id, rank_successor, id_successor);	
	}
	else if(id < root->node_ID){
		node_t* tmp = root->left ;
		if ( id <= tmp->node_ID){
			*rank_successor = tmp->true_rank ;
			*id_successor	= tmp->node_ID ;
			return 0;
		}
		else
			find_successor(tmp, root_, id, rank_successor, id_successor);
	}
	else{
		//node_t *target_node = NULL;
	 	//find_low_id(root, &target_node);
		//*rank_successor = root -> true_rank;
	}
	return 1;		
}



void Server_BST_display(node_t *root){
	if (root != NULL){
		Server_BST_display(root->left);
		printf("true_rank = %d\t", root->true_rank);fflush(stdout);
		printf("hash = %ud ", root->node_ID);
		Server_BST_display(root->right);
	}
	printf("\n");//fflush(stdout);
}

/*
node_t* root = NULL;

int main(){
*/
	
/*	for(int rank = 0; rank < 4; rank++){
		//int rank = 3 ;
		uint32_t server_hash_id = 0;
        	char server [100];
        	//printf("server = %s\n", server);
        	sprintf(server, "server with rank %d", rank);

        	//printf("server = %s length = %ld\n", server, strlen(server));
        	int len = strlen(server);
        	uint32_t seed = SEED;
        	MurmurHash3_x86_32(server, len, seed, &server_hash_id);
		//printf();
		root = add_server(root, rank, server_hash_id);

	}
*/
	//root = add_server(root, 0, 0);
//	Server_BST_display(root);

/*	Initialize_ring(&root);

	return 0 ;
}*/

