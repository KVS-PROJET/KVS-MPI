all : clean run

libkvs.so : kvs.c murmur3.c
	gcc -fPIC -shared kvs.c murmur3.c -o libkvs.so

run : main.c libkvs.so
	mpicc  main.c -I. kvs_mpi.c -L. -lkvs -o run 
	export LD_LIBRARY_PATH=./ ; mpirun -np 20 ./run 10000
clean :
	rm -rf *.o *.so run


