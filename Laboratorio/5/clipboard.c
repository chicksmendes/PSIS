#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"

 
int main(){
	char file_name[100];
	
	// Open outbound fifo
	sprintf(file_name, "./%s", OUTBOUND_FIFO);
	unlink(file_name);
	if(mkfifo(file_name, 0666)==-1){
		printf("Error creating out fifo\n");
		exit(-1);
	}
	int fifo_out = open(file_name, O_RDWR);
	if(fifo_out == -1){
		printf("Error opening in fifo\n");
		exit(-1);
	}
	
	// Open inbound fifo
	sprintf(file_name, "./%s", INBOUND_FIFO);
	unlink(file_name);
	if(mkfifo(file_name, 0666)==-1){
		printf("Error creating in fifo\n");
		exit(-1);
	}
	int fifo_in = open(file_name, O_RDWR);
	if(fifo_in == -1){
		printf("Error opening in fifo\n");
		exit(-1);
	}
	
	//int len_data;

	Message_struct messageReceived, messageSend;
	char *clipboard[10];
	char *data = NULL;

	// Init the clipboard struct
	for (int i = 0; i < 10; ++i)
	{
		clipboard[i] = NULL;
	}

	while(1){
		printf(".\n");
		
		// Reads the action that will take
		read(fifo_in, &messageReceived, sizeof(Message_struct));

		if(messageReceived.action == COPY) {
			// Allocs memory to store new data
			data = (char *)malloc(sizeof(messageReceived.data));
			if(data == NULL) {
				printf("ERROR ALOCATING MEMORY\n");
			}	

			// Copy the message from the buffer to the clipboard
			strcpy(data, messageReceived.data);

			// Verification of ID
			if(clipboard[messageReceived.region] != NULL) {
				printf("region with stuff");
				free(clipboard[messageReceived.region]);
			}

			// Assigs new data to the clipboard
			clipboard[messageReceived.region] = data;		
			printf("data stored: %s\n", clipboard[messageReceived.region]);
		}
		else if(messageReceived.action = PASTE) {
			// Copies the data from the clipboard to the buffer
			strcpy(messageSend.data, clipboard[messageReceived.region]);
			printf("data sent: %s\n", clipboard[messageReceived.region]);

			// Sends the data to the client
			write(fifo_out, &messageSend, sizeof(Message_struct));
		}
		else {
			break;
		}
	}
		
	exit(0);
	
}
