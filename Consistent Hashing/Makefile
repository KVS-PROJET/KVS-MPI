
all : clean test

test: main.c Consistent_Hashing.c mpi_const_hash.c local_kvs.c
	mpicc -I. main.c Consistent_Hashing.c mpi_const_hash.c local_kvs.c -o test
	mpirun -np 10 ./test

clean:
	rm -rf *.o test
