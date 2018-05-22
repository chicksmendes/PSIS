#ifndef THREADS_H
#define THREADS_H


#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>

#define NUMBEROFPOSITIONS 10
#define WAIT 2
#define PASTE 1
#define COPY 0

typedef struct _message {
	size_t size;
	int region;
	int action;
} Message_struct;

typedef struct _thread_info_struct {
	pthread_t thread_id; 
	int inputArgument;
	struct _thread_info_struct *next;
} thread_info_struct;

int writeAll(int sock_fd, char *buf, int len);

int readAll(int sock_fd, char *buf, int len);

void * clientThread(void * arg);

#endif