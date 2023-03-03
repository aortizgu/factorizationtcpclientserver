all: libfactorization factorization_client factorization_server

libfactorization: libfactorization.c
	gcc -Wall -Werror -O0 -g3 -c -fPIC libfactorization.c
	gcc -shared -o libfactorization.so libfactorization.o

factorization_client: factorization_client.c common.c messageprocessor.c clientconnection.c blockingqueue.c
	gcc -Wall -Werror -O0 -g3 factorization_client.c common.c messageprocessor.c clientconnection.c blockingqueue.c -o factorization_client -I. -pthread
	gcc -Wall -Werror -O0 -g3 factorization_client.c common.c messageprocessor.c clientconnection.c blockingqueue.c -o factorization_client_debug -I. -pthread -D DEBUG

factorization_server: factorization_server.c common.c thpool.c messageprocessor.c serverconnection.c
	gcc -Wall -Werror -O0 -g3 factorization_server.c common.c thpool.c messageprocessor.c serverconnection.c -o factorization_server -I. -L. -pthread -lnsl -lfactorization
	gcc -Wall -Werror -O0 -g3 factorization_server.c common.c thpool.c messageprocessor.c serverconnection.c -o factorization_server_debug -I. -L. -pthread -lnsl -lfactorization -D DEBUG

clean:
	rm *.o