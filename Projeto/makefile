CFLAGS= -g -Wall -std=gnu11

default: allin

allin: clipboard app

clipboard:  threads.c clipboard.c library.c
	gcc -c threads.c -lpthread -I.
	gcc -c library.c
	gcc  clipboard.c -lpthread -I. threads.o  library.o -g -o clipboard $(CFLAGS)

app:  clipboard.c clipboard.h library.c app_teste.c
	gcc  app_teste.c   -g -o app $(CFLAGS)

clean:
	rm -f *.o app clipboard
