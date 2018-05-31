GFLAGS= -g -Wall -std=gnu11
CFLAGS= -O2 -Wall -std=gnu11

default: allin

allin: opt app1 app2

opt: threads.c clipboard.c library.c
	gcc  threads.c library.c clipboard.c -o clipboard  -lpthread -I -O2 -Wall -std=gnu11

debug:  threads.c clipboard.c library.c
	gcc -c threads.c $(CFLAGS) -lpthread -I.
	gcc -c library.c $(CFLAGS)
	gcc  clipboard.c -lpthread -I. threads.o  library.o -o clipboard $(CFLAGS)

app1:  library.c sendFiles.c
	gcc  sendFiles.c library.c -o app1 $(GFLAGS)

app2:  library.c sendAll.c
	gcc  sendAll.c library.c -o app2 $(CFLAGS)

clean:
	rm -f *.o app1 app2 clipboard
