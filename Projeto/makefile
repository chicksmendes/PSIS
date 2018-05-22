hellomake: library.c app_teste.c clipboard.h clipboardInter.h clipboard.c utils.c utils.h
	gcc clipboard.c -o clipboard -g -lpthread -I.
	gcc -c app_teste.c library.c
	gcc -c app_teste2.c library.c
	gcc -c app_teste3.c library.c
	gcc app_teste.o library.o -o teste
	gcc app_teste2.o library.o -o teste2
	gcc app_teste3.o library.o -o teste3