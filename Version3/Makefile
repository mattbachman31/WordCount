CFLAGS=-Wall -std=c11 -fPIC

all: word-count word-countd

word-count:
	gcc $(CFLAGS) client.c -o word-count

word-countd: server.o word-count.o word-store.o
	gcc $(CFLAGS) server.o word-store.o word-count.o -ldl -o word-countd

word-count.o:
	gcc -c $(CFLAGS) word-count.c

word-store.o:
	gcc -c $(CFLAGS) word-store.c

server.o:
	gcc -c $(CFLAGS) server.c

clean:
	rm word-countd word-count prj3fifo *.o *.mod *~
