hellomake: library.c app_teste.c clipboard.h clipboard.c clipboard_backup.c
	gcc clipboard.c -o clipboard -I.
	gcc clipboard_backup.c -o clipboard_backup -I.
	gcc -c app_teste.c library.c
	gcc app_teste.o library.o -o teste