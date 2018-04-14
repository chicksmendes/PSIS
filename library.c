#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"

int clipboard_connect(char * clipboard_dir){
	char socket_name[100];
	// Socket structs
	struct sockaddr_un server_addr;
	struct sockaddr_un client_addr;


	// Socket creation
	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock == -1) {
		perror("socket");
		exit(-1);
	}
	
	printf("Socket opened\n");

	/*int err = bind(sock, (struct sockaddr *)&client_addr, sizeof(client_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}*/

	// Atribution of the name of the socket
	sprintf(socket_name, "%s%s", clipboard_dir, SOCKET_ADDR);

	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, socket_name);

	int err_c = connect(sock, (const struct sockaddr *)&server_addr, sizeof(server_addr));
	if(err_c == -1) {
		printf("Error connecting\n");
		exit(-1);
	}

	printf("Conected to server\n");

	return sock;
}

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count){
	Message_struct message;
	int numberOfCopy = 0;
	int status = 0;

	if(region > NUMBEROFPOSITIONS) {
		return 0;
	}

	// Loads the stuct with the information
	message.size = count;
	message.region = region;
	message.action = COPY;

	write(clipboard_id, &message, sizeof(Message_struct));

	read(clipboard_id, &status, sizeof(int));

	if(status == 0) {
		return 0;
	}
	if((numberOfCopy = write(clipboard_id, buf, count*sizeof(char))) == count*sizeof(char)) {
		return numberOfCopy;
	}
	else {
		return 0;
	}
	
}
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){
	Message_struct message;
	int action = PASTE;
	int numberOfBytesReceived = 0;
	int status = 0;

	char datas[10];

	message.size = count;
	message.region = region;
	message.action = PASTE;

	// Write the message with the action
	write(clipboard_id, &message, sizeof(Message_struct));

	// Reads if it can paste the content to the client
	read(clipboard_id, &status, sizeof(int));

	if(status == 0) {
		printf("STATUS 0\n");
		return 0;
	}

	// Reads the size of the data
	read(clipboard_id, &message.size, sizeof(int));

	printf("message.size %d\n", (int )message.size);

	memset(buf, '\0', message.size);

	printf("buf %s\n", (char *) buf);

	// Pastes the data to the client
	numberOfBytesReceived = read(clipboard_id, &datas, message.size*sizeof(char));

	printf("received datas: %s\n", datas);

	printf("received library: %s\n", (char *) buf);

	return numberOfBytesReceived;
}
