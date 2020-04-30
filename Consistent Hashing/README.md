Nous avons essayé d'implémenté une version simple de consistent hashing,

Si on a bien compris le principe, l'objectif est de diviser la charge de KVS uniformèment sur les serveurs en associant à chaque ensemble de clés un serveur qui stockera ces clés-valeurs et gérera par la suite les requetes get/put, en plus de ça, l'implémentation doit gérer la situation où un serveur tembe en panne ou si on voudrait ajouter un serveur, à ce moment là certaines données doivent etre déplacé vers d'autres serveurs pour ne pas perder éventuellement des données ainsi que de garder leurs distribution uniforme : le hachage consistent nous assure que le nombre des données à déplacer est plus faible, dans le cas où le hachage consistent n'est pas adopté presque tous les clés doivent etre déplacé vers d'autres serveurs autres que les premiers qui leurs été attribués, cela due au fait qu'on introduit le modulo % N en congruant la valeur hash des clés (pour déteminer le serveur qui prendra en charge cette clé) , où N est le nombre de serveurs. Si on ajoute un serveur (ou on le supprime), la formule devient (% N + 1) et la valeur hash obtenu sera différent pour tous les clés et donc tous les clés-vleurs doivent etre déplacées.

Dans cette implémentation nous avons définie, une structure d'arbre binaire BST qui va contenir les serveurs ordonnés par leur valeurs hash (obtenu par murmurHash); les clés et les serveurs sont donc hashé par la meme fonction de hachage murmurHash, et placés d'une manière abstraite dans une strucure d'anneau.

La structure BST des serveurs nous permettra de chercher le serveur responsabl d'une telle clé (clé-valeur) en déterminant le successeur de la valeur hash de la clé : les fonctions permettant la création, et la manipulation de cette structure sont dans le fichier : Consistent_Hashing.c .

Chaque serveur a donc son propre local KVS sous forme d'arbre binaire , défine dans le fichier local_kvs.c .

mpi_const_hash.c contient les fonctions à base de MPI permettant d'envoyer / traiter les requetes des clients.

La duplication des valeurs hash des serveurs n'est pas faite dans cette implémentation, en faite cette technique permet d'encore bien distribuer la charge sur les serveurs.
