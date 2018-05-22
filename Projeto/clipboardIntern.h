#ifndef CLIPBOARDINTER_H
#define CLIPBOARDINTER_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "threads.h"



#define SOCKET_ADDR "./CLIPBOARD_SOCKET"
#define ONLINE_FLAG "-c"

#define LOCAL 0
#define ONLINE 1

typedef struct _clipboard {
	char *data;
	size_t size;
} clipboard_struct;

void ctrl_c_callback_handler(int signum);

int randomPort();

#endif