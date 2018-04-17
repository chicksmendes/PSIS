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

struct sockaddr_un connect_unix(struct sockaddr_un client_addr) {


	return client_addr;
}
 
int main(int argc, char const *argv[]) {
	int modeOfFunction = 0;
	int error = 0;
	int success = 1;

	// Socket structs
	struct sockaddr_un local_addr;
	struct sockaddr_un client_addr;
	struct sockaddr_in backup_addr;
	socklen_t size_addr;

	char ip[14];
	int port = 0;
	
	// Atach the ctrl_c_callback_handler to the SIGINT signal
	signal(SIGINT, ctrl_c_callback_handler);

	if(argc == 1) {
		printf("Only local host\n");
		modeOfFunction = 0;
	}
	else if(argc == 4) {
		// Server will function only as backup
		if(strcmp(argv[1], BACKUP_FLAG) != 0) {
			printf("Incorrect initialization\n");
			exit(0);
		}
		else {
			printf("Server with backup\n");
			modeOfFunction = 1;
		}
	
	
		// Copy IP from the argv
		strcpy(ip, argv[2]);

		// Copy port from argv
		port = atoi(argv[3]);
	}
	else {
		printf("Incorrect initialization - number of arguments\n");
		exit(0);
	}

	// Create socket unix
	int sock_fd_unix = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock_fd_unix == -1) {
		perror("socket");
		exit(-1);
	}

	local_addr.sun_family = AF_UNIX;
	strcpy(local_addr.sun_path, SOCKET_ADDR);

	// Bind
	int err_unix = bind(sock_fd_unix, (struct sockaddr *) &local_addr, sizeof(local_addr));
	if(err_unix == -1) {
		perror("bind");
		exit(-1);
	}

	// Listen
	if(listen(sock_fd_unix, 5) == -1) {
		perror("listen");
		exit(-1);
	}


	// Create socket inet
	if(modeOfFunction == 1) {
		int sock_fd_inet = socket(AF_INET, SOCK_STREAM, 0);
		if(sock_fd_inet == -1) {
			perror("socket");
			exit(-1);
		}

		backup_addr.sin_family = AF_INET;
		backup_addr.sin_port= htons(port);
		inet_aton(ip, &backup_addr.sin_addr);

		// Bind
		/*int err_inet = bind(sock_fd_inet, (struct sockaddr *) &local_addr, sizeof(local_addr));
		if(err_inet == -1) {
			perror("bind");
			exit(-1);
		}*/

		if( -1 == connect(sock_fd_inet, (const struct sockaddr *) &backup_addr, sizeof(backup_addr))) {
				printf("Error connecting to backup server\n");
				exit(-1);
		}
	}

	// Clipboard data
	char *clipboard[10];
	// New data received
	char *data = NULL;

	// Structs to communicate with the client
	Message_struct messageReceived, messageSend;

	// Structs to communicate with the parent clibpoard 
	Message_struct_clipboard messagebackup;

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
		int client =  accept(sock_fd_unix, (struct sockaddr *) &client_addr, &size_addr);
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
			
				if(modeOfFunction == 1) {

				}
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
		
	close(sock_fd_inet);

	exit(0);
	
}
