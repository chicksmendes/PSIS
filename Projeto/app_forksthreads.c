#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#define MESSAGE_SIZE 10000

void *ola(void * xx){
	int i; 
	char palavra[10]; 
	int fd = clipboard_connect("./");
	if(fd== -1) {
		printf("Clipboard is off\n");
		exit(-1);
	}

	for (i=0; i<10; i++) {
		clipboard_copy(fd , i, palavra, 10);
		clipboard_paste(fd, i, palavra, 10); 
	}

	return NULL;
}

int main(){
	int i, a; 
	pthread_t thread_id[10];

	for (i=0; i < 6; i++) {
		fork();
	}

	for (i=0; i<10; i++) {
		pthread_create(&(thread_id[i]), NULL, ola, (void *) &i);
	}

	for (int j=0; j<10; j++) {
		pthread_join((thread_id[i]), (void *) &a);
	}
}
