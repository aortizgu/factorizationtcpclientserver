# factorizationtcpclientserver

compile:
```
make all
```
before execute the binaries, set LD_LIBRARY_PATH to the output folder:
```
export LD_LIBRARY_PATH=.
```
memcheck factorization_server
```
valgrind --leak-check=full --track-origins=yes ./factorization_server
```