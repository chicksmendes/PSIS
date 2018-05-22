#ifndef CLIPBOARDINTER_H
#define CLIPBOARDINTER_H


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


#include "threads.h"
#include <signal.h>

#define NUMBEROFPOSITIONS 10
#define WAIT 2
#define PASTE 1
#define COPY 0

#define SOCKET_ADDR "./CLIPBOARD_SOCKET"
#define ONLINE_FLAG "-c"

#define LOCAL 0
#define ONLINE 1

typedef struct _clipboard {
	char *data;
	size_t size;
} clipboard_struct;

typedef struct _message {
	size_t size;
	int region;
	int action;
} Message_struct;

#endif