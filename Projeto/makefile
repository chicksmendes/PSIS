CFLAGS= -g -Wall -std=gnu11

default: allin

allin: clipboard app

clipboard:  threads.c clipboard.c library.c
	gcc -c threads.c -g -lpthread -I.
	gcc -c library.c -g
	gcc  clipboard.c -lpthread -I. threads.o  library.o -g -o clipboard $(CFLAGS)

app:  library.c app_teste.c
	gcc  app_teste.c library.c -g -o app $(CFLAGS)

clean:
	rm -f *.o app clipboard
