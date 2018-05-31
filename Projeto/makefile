GFLAGS= -g -Wall -std=gnu11
CFLAGS= -O2 -Wall -std=gnu11

default: allin

allin: app1 app2 app3 app4 opt

opt: threads.c clipboard.c library.c
	gcc  threads.c library.c clipboard.c -lpthread -I. -o clipboard -O2 -Wall -std=gnu11

debug:  threads.c clipboard.c library.c
	gcc -c threads.c $(CFLAGS) -lpthread -I.
	gcc -c library.c $(CFLAGS)
	gcc  clipboard.c -lpthread -I. threads.o  library.o -o clipboard $(CFLAGS)

app1:  library.c app_teste.c
	gcc  app_teste.c library.c -o app1 $(GFLAGS)

app2:  library.c app_files.c
	gcc  app_files.c library.c -o app2 $(GFLAGS)

app3:  library.c app_teste3.c
	gcc  app_teste3.c library.c -o app3 $(CFLAGS)

app4: library.c files.c
	gcc  files.c library.c -o app4 $(GFLAGS)

clean:
	rm -f *.o app1 app2 app3 app4 clipboard
