all: libfactorization factorization_client factorization_server

libfactorization: libfactorization.c
	gcc -c -fPIC libfactorization.c
	gcc -shared -o libfactorization.so libfactorization.o

factorization_client: factorization_client.c common.c
	gcc factorization_client.c common.c -o factorization_client -I.

factorization_server: factorization_server.c
	gcc factorization_server.c common.c -o factorization_server -I. -L. -lfactorization

clean:
	rm *.o