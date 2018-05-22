#ifndef CLIPBOARDINTER_H
#define CLIPBOARDINTER_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "threads.h"

#define SOCKET_ADDR "./CLIPBOARD_SOCKET"
#define ONLINE_FLAG "-c"


void ctrl_c_callback_handler(int signum);

int randomPort();

void connectUnix();

void connect_inet(int portDown);

void connect_inetIP(int port, char ip[]);

void createPipe();
#endif