CFLAGS=-Wall -std=c11 -D_XOPEN_SOURCE=500

all: word-count word-countd

word-count: client.c
	gcc $(CFLAGS) -lrt client.c -o word-count

word-countd: server.o word-count.o word-store.o
	gcc $(CFLAGS) -lrt server.o word-store.o word-count.o -o word-countd

word-count.o:
	gcc -c $(CFLAGS) word-count.c

word-store.o:
	gcc -c $(CFLAGS) word-store.c

server.o: server.c
	gcc -c $(CFLAGS) server.c

clean:
	rm -f word-countd word-count *.o *.mod *~
