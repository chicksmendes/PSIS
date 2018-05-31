#ifndef CLIPBOARDINTER_H
#define CLIPBOARDINTER_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "threads.h"

#define ONLINE_FLAG "-c"

/**
 * [ctrl_c_callback_handler description]
 * @param signum [description]
 */
void ctrl_c_callback_handler(int signum);

/**
 * Cria um porto aleatorio
 * @return [description]
 */
int randomPort();

/**
 * Coneta-se a um socket unix
 */
void connectUnix();

/**
 * coneta se a um socket inet
 * @return sock_fd do socket
 */
int connect_inet();

/**
 * Coneta se a um socket com um ip determinado
 * @param port porto a que nos queremos conetar
 * @param ip   ip da maquina que nos queremos ligar
 */
void connect_inetIP(int port, char ip[]);

/**
 * Cria um pipe
 */
void createPipe();

/**
 * Inicia as regiões do clipboard
 */
void initClipboard();

#endif