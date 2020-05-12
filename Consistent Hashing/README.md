Nous avons essayé d'implémenté une version simple de consistent hashing,

Si on a bien compris le principe, l'objectif est de diviser la charge de KVS uniformèment sur les serveurs en associant à chaque ensemble de clés un serveur qui stockera ces clés-valeurs et gérera par la suite les requetes get/put, en plus de ça, l'implémentation doit gérer la situation où un serveur tembe en panne ou si on voudrait ajouter un serveur, à ce moment là certaines données doivent etre déplacé vers d'autres serveurs pour ne pas perder éventuellement des données ainsi que de garder leurs distribution uniforme : le hachage consistent nous assure que le nombre des données à déplacer est plus faible, dans le cas où le hachage consistent n'est pas adopté presque tous les clés doivent etre déplacé vers d'autres serveurs autres que les premiers qui leurs été attribués, cela due au fait qu'on introduit le modulo % N en congruant la valeur hash des clés (pour déteminer le serveur qui prendra en charge cette clé) , où N est le nombre de serveurs. Si on ajoute un serveur (ou on le supprime), la formule devient (% N + 1) et la valeur hash obtenu sera différent pour tous les clés et donc tous les clés-vleurs doivent etre déplacées.

Dans cette implémentation nous avons définie, une structure d'arbre binaire BST qui va contenir les serveurs ordonnés par leur valeurs hash (obtenu par murmurHash); les clés et les serveurs sont donc hashé par la meme fonction de hachage murmurHash, et placés d'une manière abstraite dans une strucure d'anneau.

La structure BST des serveurs nous permettra de chercher le serveur responsabl d'une telle clé (clé-valeur) en déterminant le successeur de la valeur hash de la clé : les fonctions permettant la création, et la manipulation de cette structure sont dans le fichier : Consistent_Hashing.c .

Chaque serveur a donc son propre local KVS sous forme d'arbre binaire , défine dans le fichier local_kvs.c .

mpi_const_hash.c contient les fonctions à base de MPI permettant d'envoyer / traiter les requetes des clients.

La duplication des valeurs hash des serveurs n'est pas faite dans cette implémentation, en faite cette technique permet d'encore bien distribuer la charge sur les serveurs.

Cette implémentation se base sur le principe de hachage consistent à base de MPI.

Pour lancer un test il faut entrer certains paramètres au moment de compilation :

nproc		: 	Nombre total de processus MPI

NBR_SERVERS 	:	Nombre de Servers initials ( doit etre < nproc / 2 )

LIMIT_HASH_SPACE:	Représente l'espace de hachage a considérer pour l'application ( doit etre >> nproc )

NBR_REQUESTS	:	Nombre de requets (insertion et recherche) souhaités (de l'ordre de 10⁶ ou plus)


---------------------------------------------------------------------------------------------------
MODE Normal :---------------------------------

Exemple d'utilisation en mode NORMAL :	(MODE=0) en tant que table de hachage distribué statique

pour compiler 	:

>> make nproc=20 MODE=0 NBR_SERVERS=4 LIMIT_HASH_SPACE=100 NBR_REQUESTS=1000000 

Description : on aura 
	-	4 serveurs,
 
	-	les valeurs hash des clés et les identifiant des serveurs dans l'arbre sont tous entre 0 et 100, 

	-	10⁶ insertions et recherches au total

----------------------------------------------------------------------------------------------------
MODE Dynamique--------------------------------

l'implémentation actuel de hashage consistent suppose que pour l'ajout d'un serveur dynamiquement on choisie au hasard un processus client et on le transforme en un serveur, et pour la suppression on supprime un serveur qui n'a pas été dans l'ensemble initial des serveurs .

Contrairement au table de hachage distribué statique où tous les clés-valeurs doivent etre déplacé vers un autre serveur lors de l'ajout/suppression d'un serveur. Dans notre implémentation qui adopte le principe de hachage consistent , au moment d'ajout / suppression d'un serveur, un petit ensemble de clés-valeurs sera déplacé vers le serveur ajouté/ vers le successeur du serveur supprimé, respectivement .

Exemple d'utilisation en mode dynamic : (MODE=1) possibilité d'ajout ou suppression d'un serveur dynamiquement

>> make nproc=20 MODE=1 NBR_SERVERS=4 LIMIT_HASH_SPACE=100 NBR_REQUESTS=1000000




