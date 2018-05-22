#ifndef THREADS_H
#define THREADS_H


#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>

#include "regions.h"

typedef struct _thread_info_struct {
	pthread_t thread_id; 
	int inputArgument;
	struct _thread_info_struct *next;
} thread_info_struct;

void * clientThread(void * arg);

#endif