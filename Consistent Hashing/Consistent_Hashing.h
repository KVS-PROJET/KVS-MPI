#ifndef _CONSISTENT_HASHING_H
#define _CONSISTENT_HASHING_H
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

typedef struct node_s{
	uint32_t node_ID ;	// La valeur hash de serveur obtenu avec MurmurHash
	int true_rank ;		// MPI id de serveur
	struct node_s *left ;
	struct node_s *right ;
} node_t ;

node_t* make_node(int rank, uint32_t new_id) ;

node_t* add_server(node_t* root, int rank , uint32_t new_id);

int find_low_id(node_t* root, node_t** target_node);

node_t* remove_server(node_t* root, uint32_t to_remove);

int search_server(node_t* root, uint32_t server_id, node_t ** target_node);

int find_successor(node_t *root, uint32_t id, int *rank_successor);

void Server_BST_display(node_t *root);

#endif

