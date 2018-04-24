#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"

int clipboard_connect(char * clipboard_dir){
	char fifo_name[100];
	
	sprintf(fifo_name, "%s%s", clipboard_dir, INBOUND_FIFO);
	int fifo_send = open(fifo_name, O_WRONLY);
	sprintf(fifo_name, "%s%s", clipboard_dir, OUTBOUND_FIFO);
	int fifo_recv = open(fifo_name, O_RDONLY);
	if(fifo_recv < 0)
		return fifo_recv;
	else
		return fifo_send;
}

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count){
	Message_struct message;
	int numberOfCopy = 0;

	if(region > NUMBEROFPOSITIONS) {
		return 0;
	}

	// Loads the stuct with the information
	strcpy(message.data, (char *) buf);
	message.region = region;
	message.action = COPY;

	if((numberOfCopy = write(clipboard_id, &message, sizeof(Message_struct))) != sizeof(Message_struct)) {
		return 0;
	}
	else {
		return numberOfCopy;
	}
	
}
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){
	Message_struct message;
	int action = PASTE;
	int numberOfBytesReceived = 0;

	message.region = region;
	message.action = PASTE;

	write(clipboard_id-1, &message, sizeof(Message_struct));

	numberOfBytesReceived = read(clipboard_id, &message, sizeof(Message_struct));

	strcpy(buf, message.data);

	return numberOfBytesReceived;
}
