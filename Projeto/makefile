CFLAGS= -g -Wall -std=gnu11

default: allin

allin: clipboard app1 app2 app3

clipboard:  threads.c clipboard.c library.c
	gcc -c threads.c -g -lpthread -I.
	gcc -c library.c -g
	gcc  clipboard.c -lpthread -I. threads.o  library.o -g -o clipboard $(CFLAGS)

app1:  library.c app_teste.c
	gcc  app_teste.c library.c -g -o app1 $(CFLAGS)

app2:  library.c app_files.c
	gcc  app_files.c library.c -g -o app2 $(CFLAGS)

app3:  library.c app_teste3.c
	gcc  app_teste3.c library.c -g -o app3 $(CFLAGS)

clean:
	rm -f *.o app clipboard
