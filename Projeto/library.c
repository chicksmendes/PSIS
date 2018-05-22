#include "clipboard.h"
#include "clipboardIntern.h"

int clipboard_connect(char * clipboard_dir){
	// Socket Structs
	struct sockaddr_un server_addr;
	struct sockaddr_un client_addr;

	// Create Socket
	int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock_fd == -1) {
		perror("socket");
		return(-1);
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
		return(-1);
	}

	printf("Conected to server\n");

	return sock_fd;
}
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count) {
	Message_struct message;
	int numberOfBytesSent = 0;
	int status = 1;

	if(region > NUMBEROFPOSITIONS-1 || region < 0) {
		return 0;
	}

	// Loads the stuct with the information
	message.region = (size_t) region;
	message.size = count;
	message.action = COPY;

	// Informs the server of the action that the client wants to take - COPY
	write(clipboard_id, &message, sizeof(Message_struct));

	if(status == 1) {
		// Sends the data to the clipboard
		numberOfBytesSent =  writeAll(clipboard_id, buf, count*sizeof(char));

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
/*
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);*/