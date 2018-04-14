#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"


// Unlinks the sockets when the program stops
void ctrl_c_callback_handler(int signum){
	printf("Caught signal Ctr-C\n");
	unlink(SOCKET_ADDR);
	exit(0);
}

 
int main(){
	char socket_name[100];
	int error = 0;
	int success = 1;

	// Socket structs
	struct sockaddr_un local_addr;
	struct sockaddr_un client_addr;
	socklen_t size_addr;

	// Atach the ctrl_c_callback_handler to the SIGINT signal
	signal(SIGINT, ctrl_c_callback_handler);

	unlink(SOCKET_ADDR);

	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock == -1) {
		perror("socket");
		exit(-1);
	}

	local_addr.sun_family = AF_UNIX;
	// Atribution of the name of the socket
	sprintf(socket_name, "./%s",  SOCKET_ADDR);

	strcpy(local_addr.sun_path, socket_name);

	// Bind
	int err = bind(sock, (struct sockaddr *) &local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}

	// Listen
	if(listen(sock, 5) == -1) {
		perror("listen");
		exit(-1);
	}

	Message_struct messageReceived, messageSend;
	char *clipboard[10];
	char *data = NULL;
	//char *dataSend = NULL;

	// Init the clipboard struct
	for (int i = 0; i < 10; i++)
	{
		clipboard[i] = NULL;
	}

	printf("Ready to accept clients\n");

	while(1){
		printf(".\n");

		size_addr = sizeof(struct sockaddr);

		int client = accept(sock, (struct sockaddr *) &client_addr, &size_addr);
		if(client == -1) {
			perror("accept");
			exit(-1);
		}

		printf("Accepted connection\n");
		while(1) {
			// Reads the action that will take
			int receivedBytesMessage =read(client, &messageReceived, sizeof(Message_struct));

			if(receivedBytesMessage == 0) {
				printf("GoodBye\n");
				break;
			}

			if(messageReceived.action == COPY) {
				printf("COPY\n");
				// Allocs memory to store new data
				data = (char *)malloc(sizeof(char)*messageReceived.size);
				if(data == NULL) {
					write(client, &error, sizeof(int));
				}	
				// Informs the client that as allocated memory to receive the data
				write(client, &success, sizeof(int));

				// Reads the data 
				read(client, data, messageReceived.size*sizeof(char));

				data[messageReceived.size-1] = '\0';
				// Verification of ID
				if(clipboard[messageReceived.region] != NULL) {
					printf("Region with stuff\n");
					free(clipboard[messageReceived.region]);
					printf("Region cleared\n");
				}

				// Assigs new data to the clipboard
				clipboard[messageReceived.region] = data;

				printf("data stored: %s\n", clipboard[messageReceived.region]);
			}
			else if(messageReceived.action == PASTE) {
				printf("PASTE\n");
				if(messageReceived.size < strlen(clipboard[messageReceived.region])+1) {
					printf("Doesn't have enough size to paste\n");
					write(client, &error, sizeof(int));
					break;
				}
				else {
					write(client, &success, sizeof(int));
				}

				messageSend.size = strlen(clipboard[messageReceived.region])+1;
				write(client, &messageSend.size, sizeof(size_t));

				// Sends the data to the client
				int numberOfBytesSend = write(client, clipboard[messageReceived.region], messageSend.size);

				printf("sent: %s, %d\n", clipboard[messageReceived.region], numberOfBytesSend);

			}
		}
	}
		
	exit(0);
	
}
