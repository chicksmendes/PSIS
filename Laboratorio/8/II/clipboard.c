#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"

int sock_fd_inet = 0;
int error = 0;
int success = 1;

// Clipboard data
clipboard_struct clipboard;
// New data received
char *data = NULL;

int modeOfFunction = 0;

// Unlinks the sockets when the program stops
void ctrl_c_callback_handler(int signum){
	printf("Caught signal Ctr-C\n");
	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		free(clipboard.clipboard[i]);
	}
	printf("free clipboard\n");
	unlink(SOCKET_ADDR);
	close(sock_fd_inet);
	exit(0);
}

int connect_unix(struct sockaddr_un *local_addr) {
	// Create socket unix
	int sock_fd_unix = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock_fd_unix == -1) {
		perror("socket");
		exit(-1);
	}

	local_addr->sun_family = AF_UNIX;
	strcpy(local_addr->sun_path, SOCKET_ADDR);

	// Bind
	int err_unix = bind(sock_fd_unix, (struct sockaddr *) local_addr, sizeof(struct sockaddr_un));
	if(err_unix == -1) {
		perror("bind");
		exit(-1);
	}

	// Listen
	if(listen(sock_fd_unix, 5) == -1) {
		perror("listen");
		exit(-1);
	}
	printf("Local socket initiated\n");
	return sock_fd_unix;
}

int connect_inet(struct sockaddr_in *backup_addr, int port, char *ip) {
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd_inet == -1) {
		perror("socket");
		exit(-1);
	}

	backup_addr->sin_family = AF_INET;
	backup_addr->sin_port= htons(port);
	inet_aton(ip, &backup_addr->sin_addr);

	// Bind
	/*int err_inet = bind(sock_fd_inet, (struct sockaddr *) &local_addr, sizeof(local_addr));
	if(err_inet == -1) {
		perror("bind");
		exit(-1);
	}*/

	if( -1 == connect(sock_fd, (const struct sockaddr *) backup_addr, sizeof(struct sockaddr_in))) {
			printf("Error connecting to backup server\n");
			exit(-1);
	}
	printf("Online socket initiated\n");
	return sock_fd;
}

clipboard_struct backup(int sock_fd_inet) {
	Message_struct messageBackup;
	messageBackup.action = BACKUP;
	int statusBackup = write(sock_fd_inet, &messageBackup, sizeof(Message_struct));
	if(statusBackup == 0) {
		printf("cannot acess backup\n");
	}

	read(sock_fd_inet, &messageBackup, sizeof(Message_struct));

	printf("Performing backup\n");
	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		if(messageBackup.size[i] != 0) {
			printf("region %d ", i);
			data = (char *)malloc(sizeof(char)*messageBackup.size[i]);
			if(data == NULL) {
				perror("malloc");
				exit(0);
			}

			// Store the size of the clipboard region
			clipboard.size[i] = messageBackup.size[i];
			int numberOfBytesBackup = read(sock_fd_inet, data, clipboard.size[i]);
			if(numberOfBytesBackup != clipboard.size[i]) {
				printf("Number of bytes received backup is Incorrect. Received %d and it should be %d\n", numberOfBytesBackup, (int ) clipboard.size[i]);
				break;
			}
			clipboard.clipboard[i] = data;
			printf("	received %s, size %d\n", clipboard.clipboard[i], numberOfBytesBackup);
		}
	}

	return clipboard;
}

int copyBackup(Message_struct messageReceived, int sock_fd_inet) {
	int statusBackup;

	write(sock_fd_inet, &messageReceived, sizeof(Message_struct));

	// Read if can send the data
	read(sock_fd_inet, &statusBackup, sizeof(int));
	if(statusBackup == 1) {
		write(sock_fd_inet, clipboard.clipboard[messageReceived.region], messageReceived.size[messageReceived.region]);
		printf("Backup new data\n");
		return 1;
	}
	else {
		printf("Can't backup data\n");
	}
	return 0;
}

int copy(Message_struct messageReceived, int client) {
	// Allocs memory to store new data
	data = (char *)malloc(sizeof(char)*messageReceived.size[messageReceived.region]);
	if(data == NULL) {
		write(client, &error, sizeof(int));
		return 0;
	}

	// Store the size of the clipboard region
	clipboard.size[messageReceived.region] = messageReceived.size[messageReceived.region];

	// Informs the client that as allocated memory to receive the data
	write(client, &success, sizeof(int));

	// Receives the data from the client
	int numberOfBytesCopied = read(client, data, clipboard.size[messageReceived.region]);

	// Erases old data
	if(clipboard.clipboard[messageReceived.region] != NULL) {
		printf("Region cleared\n");
		free(clipboard.clipboard[messageReceived.region]);
	}

	// Assigns new data to the clipboard
	clipboard.clipboard[messageReceived.region] = data;

	printf("Received %d bytes - data: %s\n", numberOfBytesCopied, clipboard.clipboard[messageReceived.region]);

	return 1;
}

int paste(Message_struct messageReceived, int client) {
	Message_struct messageSend;
	if(messageReceived.size[messageReceived.region] < clipboard.size[messageReceived.region]) {
		write(client, &error, sizeof(int));
	}
	else {
		write(client, &success, sizeof(int));

		// Loads the structure with the information to the client
		messageSend.action = PASTE;
		messageSend.size[messageReceived.region] = clipboard.size[messageReceived.region];
		messageSend.region = messageReceived.region;

		write(client, &messageSend, sizeof(Message_struct));

		// Sends the data to the client
		int numberOfBytesPaste = write(client, clipboard.clipboard[messageSend.region], clipboard.size[messageReceived.region]);
		printf("Sent %d bytes - data: %s\n", numberOfBytesPaste, clipboard.clipboard[messageSend.region]);
	}

	return 1;
}
 

void * clientThread(void * threadInput) {
	int receivedBytesMessage = 1;
	int client = *((int *) threadInput);
	Message_struct messageReceived;

	// Communication with the client
	while(receivedBytesMessage != 0) {
		// Reads the inbood message from the client
		receivedBytesMessage = read(client, &messageReceived, sizeof(Message_struct));

		// If client sends EOF, terminates connection
		if(receivedBytesMessage == 0) {
			break;
		}

		if(messageReceived.action == COPY) {
			printf("\nCOPY\n");
			if(copy(messageReceived, client) == 0) {
				printf("Error on copy\n");
				break;
			}

			// Performs a backup of the information
			if(modeOfFunction == 1) {
				if(copyBackup(messageReceived, sock_fd_inet) == 0) {
					printf("Error backing up new data\n");
				}
			}
		}
		else if(messageReceived.action == PASTE) {
			//printf("Received information - action: PASTE\n");
			printf("\nPASTE\n");

			if(paste(messageReceived, client) == 0) {
				printf("Error on pasting\n");
			}
		}
	}
	printf("GoodBye\n");
}



int main(int argc, char const *argv[]) {
	int statusBackup = 0;

	// Socket structs
	struct sockaddr_un local_addr;
	struct sockaddr_un client_addr;
	struct sockaddr_in backup_addr;
	socklen_t size_addr;

	char ip[14];
	int port = 0;

	// Structs to communicate with the client
	Message_struct messageReceived, messageSend;
	// Structs to communicate with the parent clibpoard 
	Message_struct messageBackup;

	pthread_t thread_id;
	
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

	//unlink(SOCKET_ADDR);

	// Create socket unix
	int sock_fd_unix = connect_unix(&local_addr);

	// Create socket inet
	if(modeOfFunction == 1) {
		sock_fd_inet = connect_inet(&backup_addr, port, ip);
	}

	// Init the clipboard struct
	for (int i = 0; i < 10; i++)
	{
		clipboard.clipboard[i] = NULL;
		clipboard.size[i] = 0;
	}

	// Loads data from Backup
	if(modeOfFunction == 1) {
		clipboard = backup(sock_fd_inet);
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
		// Creates new thread to handle the comunicatuion with the client
		pthread_create(&thread_id, NULL, &clientThread, &client);
		printf("Thread created - ID %lu\n",  thread_id);

	}

	exit(0);
}

