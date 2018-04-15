#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "clipboard.h"

int clipboard_connect(char * clipboard_dir){
	char socketName[100];

	// Socket Structs
	struct sockaddr_un server_addr;
	struct sockaddr_un client_addr;

	// Create Socket
	int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock_fd == -1) {
		perror("socket");
		exit(-1);
	}

	printf("Socket opened\n");
	
	// Atribution of a name to the socket
	// Client
	client_addr.sun_family = AF_UNIX;
	sprintf(client_addr.sun_path, "%ssocket_%d", clipboard_dir, getpid());
	// Server
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, SOCKET_ADDR);

	int err_c = connect(sock_fd, (const struct sockaddr *) &server_addr, sizeof(struct sockaddr_un));
	if(err_c == -1) {
		printf("Error connecting\n");
		exit(-1);
	}

	printf("Conected to server\n");

	return sock_fd;
}

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count){
	Message_struct message;
	int numberOfBytesSent = 0;
	int status = 0;

	if(region > NUMBEROFPOSITIONS) {
		return 0;
	}

	// Loads the stuct with the information
	message.size = count;
	message.region = region;
	message.action = COPY;

	// Informs the server of the action that the client wants to take - COPY
	write(clipboard_id, &message, sizeof(Message_struct));

	// Reads if the server is ready for it to continue
	read(clipboard_id, &status, sizeof(int));

	if(status == 1) {
		// Sends the data to the clipboard
		numberOfBytesSent =  write(clipboard_id, buf, count*sizeof(char));

		// Tests if all the information was sent
		if(numberOfBytesSent != count*sizeof(char)) {
			printf("Didn't send all the information to the clipboard\n");
			return 0;
		}
	}
	else {
		// Server cannot received data
		printf("Server cannot received data\n");
		return 0;
	}
		
	//printf("Sent %d bytes - data: %s\n", numberOfBytesSent, (char *) buf);

	return numberOfBytesSent;
}
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){
	Message_struct message;
	int numberOfBytesReceived = 0;
	int status = 0;

	if(region > NUMBEROFPOSITIONS) {
		printf("Paste region out of bounds\n");
		return 0;
	}

	// Loads the stuct with the information
	message.size = count;
	message.region = region;
	message.action = PASTE;

	// Informs the server of the action that the client wants to take - PASTE
	write(clipboard_id, &message, sizeof(Message_struct));

	// Reads if the slient has enough space to store the PASTE data
	read(clipboard_id, &status, sizeof(int));
	if(status == 1) {
		// Reads the information about the message stored at the cliboard
		read(clipboard_id, &message, sizeof(Message_struct));

		// Reads the data from the clipboard
		numberOfBytesReceived = read(clipboard_id, buf, message.size*sizeof(char));

		//printf("Received %d bytes - data: %s\n", numberOfBytesReceived, (char *) buf);
	}
	else {
		return 0;
	}
	return numberOfBytesReceived;
}
