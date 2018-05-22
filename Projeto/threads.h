#ifndef THREADS_H
#define THREADS_H


#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUMBEROFPOSITIONS 10
#define WAIT 2
#define PASTE 1
#define COPY 0

#define LOCAL 0
#define ONLINE 1

#define CLIPBOARD 1
#define APP 0

typedef struct _clipboard {
	char *data;
	size_t size;
} clipboard_struct;

typedef struct _message {
	size_t size;
	int region;
	int action;
} Message_struct;

typedef struct _thread_info_struct {
	pthread_t thread_id; 
	int inputArgument;
	int type;
	struct _thread_info_struct *next;
} thread_info_struct;

int writeAll(int sock_fd, char *buf, int len);

int readAll(int sock_fd, char *buf, int len);

int copy(Message_struct messageReceived, int client, int type);

int paste(Message_struct messageReceived, int client, int type);

int readUp(int client, void * data, size_t size);

int writeUp(int client, Message_struct message, char * data);

void * clientThread(void * arg);

void * upThread(void * arg);

void * downThread(void * arg);


#endif