all: libfactorization factorization_client factorization_server

libfactorization: libfactorization.c
	gcc -c -fPIC libfactorization.c
	gcc -shared -o libfactorization.so libfactorization.o

factorization_client: factorization_client.c common.c messageprocessor.c
	gcc factorization_client.c common.c messageprocessor.c -o factorization_client -I.

factorization_server: factorization_server.c common.c thpool.c messageprocessor.c serverconnection.c
	gcc factorization_server.c common.c thpool.c messageprocessor.c serverconnection.c -o factorization_server -I. -L. -pthread -lnsl -lfactorization

clean:
	rm *.o