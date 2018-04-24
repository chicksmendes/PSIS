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
	int error = 0;
	int success = 1;

	// Socket structs
	struct sockaddr_un local_addr;
	struct sockaddr_un client_addr;
	socklen_t size_addr;
	
	// Atach the ctrl_c_callback_handler to the SIGINT signal
	signal(SIGINT, ctrl_c_callback_handler);

	// Create socket
	int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock_fd == -1) {
		perror("socket");
		exit(-1);
	}

	local_addr.sun_family = AF_UNIX;
	strcpy(local_addr.sun_path, SOCKET_ADDR);

	// Bind
	int err = bind(sock_fd, (struct sockaddr *) &local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}

	// Listen
	if(listen(sock_fd, 5) == -1) {
		perror("listen");
		exit(-1);
	}

	// Clipboard data
	char *clipboard[10];
	// New data received
	char *data = NULL;

	// Structs to communicate with the client
	Message_struct messageReceived, messageSend;

	// Init the clipboard struct
	for (int i = 0; i < 10; i++)
	{
		clipboard[i] = NULL;
	}

	printf("Ready to accept clients\n");

	while(1){
		printf(".\n");

		// Reset hold variable
		size_addr = sizeof(struct sockaddr);

		// Accept client to communicate
		int client =  accept(sock_fd, (struct sockaddr *) &client_addr, &size_addr);
		if(client == -1) {
			perror("accept");
			exit(-1);
		}

		printf("Accepted connection\n");

		// Communication with the client
		while(1) {
			// Reads the inbood message from the client
			int receivedBytesMessage = read(client, &messageReceived, sizeof(Message_struct));

			// If client sends EOF, terminates connection
			if(receivedBytesMessage == 0) {
				printf("GoodBye\n");
				break;
			}

			if(messageReceived.action == COPY) {
				//printf("Received information - action: COPY\n");
				printf("\nCOPY\n");
				// Allocs memory to store new data
				data = (char *)malloc(sizeof(char)*messageReceived.size);
				if(data == NULL) {
					write(client, &error, sizeof(int));
					break;
				}	
				// Informs the client that as allocated memory to receive the data
				write(client, &success, sizeof(int));

				// Receives the data from the client
				int numberOfBytesCopied = read(client, data, messageReceived.size*sizeof(char));

				// Erases old data
				if(clipboard[messageReceived.region] != NULL) {
					printf("Region cleared\n");
					free(clipboard[messageReceived.region]);
				}

				// Assigns new data to the clipboard
				clipboard[messageReceived.region] = data;

				printf("Received %d bytes - data: %s\n", numberOfBytesCopied, clipboard[messageReceived.region]);
			}
			else if(messageReceived.action == PASTE) {
				//printf("Received information - action: PASTE\n");
				printf("\nPASTE\n");
				// Confirms if the client has enough space to store the PASTE data
				if(messageReceived.size < strlen(clipboard[messageReceived.region]) + 1) {
					write(client, &error, sizeof(int));
				}
				else {
					write(client, &success, sizeof(int));

					// Loads the structure with the information to the client
					messageSend.action = PASTE;
					messageSend.size = strlen(clipboard[messageReceived.region]) + 1;
					messageSend.region = messageReceived.region;

					write(client, &messageSend, sizeof(Message_struct));

					// Sends the data to the client
					int numberOfBytesPaste = write(client, clipboard[messageSend.region], messageSend.size*sizeof(char));
				
					printf("Sent %d bytes - data: %s\n", numberOfBytesPaste, clipboard[messageSend.region]);
				}
			}
		}

		/*read(fifo_in, data, 10);
		printf("received %s\n", data);
		len_data = strlen(data);
		printf("sending value %d - legth %d\n", len_data, sizeof(len_data));
		write(fifo_out, &len_data, sizeof(len_data));*/
	}
		
	exit(0);
	
}
