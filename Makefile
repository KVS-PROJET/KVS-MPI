nproc		  = 	20
MODE		  =	0

NBR_SERVERS	  =	4
LIMIT_HASH_SPACE  =	100
NBR_REQUESTS	  =	1000000


CC = mpicc
CFLAGS = -fPIC 
LDFLAGS = -shared 

SRCS = Consistent_Hashing.c murmur3.c mpi_const_hash.c local_kvs.c
target_lib = libMPIConstHash.so
	
all : clean run

$(target_lib) :	${SRCS}
	$(CC) ${CFLAGS} ${LDFLAGS} -I. $^ -o $@

run : main.c $(target_lib)

ifeq ($(MODE),1) 
	@echo "Mode $(Mode)  :  Dynamic mode"
	$(CC) main.c -D_DYNAMIC_ADD_REMOVE_SERVER_ -I. -L. -lMPIConstHash -o $@
	export LD_LIBRARY_PATH=./ ; mpirun -np $(nproc) ./run $(NBR_SERVERS) $(LIMIT_HASH_SPACE) $(NBR_REQUESTS)

else
	@echo "Mode $(Mode)  :  Normal mode"
	$(CC) main.c -I. -L. -lMPIConstHash -o $@
	export LD_LIBRARY_PATH=./ ; mpirun -np $(nproc) ./run $(NBR_SERVERS) $(LIMIT_HASH_SPACE) $(NBR_REQUESTS)

endif


clean :
	rm -rf *.o *.so run


