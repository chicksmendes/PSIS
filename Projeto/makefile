CFLAGS= -g -Wall -std=gnu11

default: allin

allin: clipboard app

clipboard: utils.c regions.c threads.c clipboard.c library.c
	gcc  clipboard.c utils.c regions.c threads.c  library.c  -lpthread -g -o clipboard $(CFLAGS)

app:  clipboard.c clipboard.h library.c app_teste.c
	gcc  clipboard.c library.c app_teste.c -g -o app $(CFLAGS)

clean:
	rm -f *.o app clipboard
