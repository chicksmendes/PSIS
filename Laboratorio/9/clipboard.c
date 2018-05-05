#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "clipboard.h"

int sock_fd_inet = 0;
int error = 0;
int success = 1;

// Clipboard data
clipboard_struct clipboard;
// New data received
char *data = NULL;

int modeOfFunction = 0;

// Socket structs
// UNIX
struct sockaddr_un local_addr_un;
struct sockaddr_un client_addr;

// INET
struct sockaddr_in local_addr_in;
struct sockaddr_in backup_addr;
struct sockaddr_in clientClipboard_addr;



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

int connect_unix(struct sockaddr_un *local_addr_un) {
	// Create socket unix
	int sock_fd_unix = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock_fd_unix == -1) {
		perror("socket");
		exit(-1);
	}

	local_addr_un->sun_family = AF_UNIX;
	strcpy(local_addr_un->sun_path, SOCKET_ADDR);

	// Bind
	int err_unix = bind(sock_fd_unix, (struct sockaddr *) local_addr_un, sizeof(struct sockaddr_un));
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

int connect_inetIP(struct sockaddr_in *backup_addr, int port, char *ip) {
	int sock_fd_inet = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd_inet == -1) {
		perror("socket");
		exit(-1);
	}

	backup_addr->sin_family = AF_INET;
	backup_addr->sin_port= htons(port);
	inet_aton(ip, &backup_addr->sin_addr);

	// Bind
	/*int err_inet = bind(sock_fd_inet, (struct sockaddr *) &local_addr_un, sizeof(local_addr_un));
	if(err_inet == -1) {
		perror("bind");
		exit(-1);
	}*/

	if( -1 == connect(sock_fd_inet, (const struct sockaddr *) backup_addr, sizeof(struct sockaddr_in))) {
			printf("Error connecting to backup server\n");
			exit(-1);
	}
	printf("Online socket initiated\n");
	return sock_fd_inet;
}


int connect_inet(struct sockaddr_in *local_addr_in, int portDown) {
	int sock_fd_inet = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd_inet == -1) {
		perror("socket");
		exit(-1);
	}


	local_addr_in->sin_family = AF_INET;
	local_addr_in->sin_port= htons(portDown);
	local_addr_in->sin_addr.s_addr= INADDR_ANY;
	int err = bind(sock_fd_inet, (struct sockaddr *) local_addr_in, sizeof(struct sockaddr_in));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}
	printf("Socket created and binded\n");

	listen(sock_fd_inet, 1);

	printf("Ready to accept connections\n");
	return sock_fd_inet;
}




void backupInit(int sock_fd_inetIP) {
	Message_struct messageBackup;
	messageBackup.action = BACKUP;
	int statusBackup = write(sock_fd_inetIP, &messageBackup, sizeof(Message_struct));
	if(statusBackup == 0) {
		printf("cannot acess backup\n");
	}

	read(sock_fd_inetIP, &messageBackup, sizeof(Message_struct));

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
			int numberOfBytesBackup = read(sock_fd_inetIP, data, clipboard.size[i]);
			if(numberOfBytesBackup != clipboard.size[i]) {
				printf("Number of bytes received backup is Incorrect. Received %d and it should be %d\n", numberOfBytesBackup, (int ) clipboard.size[i]);
				break;
			}
			clipboard.clipboard[i] = data;
			printf("	received %s, size %d\n", clipboard.clipboard[i], numberOfBytesBackup);
		}
	}
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
 
int backup(Message_struct messageReceived, int client) {
	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		messageReceived.size[i] = clipboard.size[i];
	}

	// Sends the amount of data present in the clipboard
	write(client, &messageReceived, sizeof(Message_struct));

	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		if(messageReceived.size[i] != 0) {
			printf("region %d ", i);
			printf("clipboard content %s size %d\n", clipboard.clipboard[i], (int ) clipboard.size[i]);
			write(client, clipboard.clipboard[i], clipboard.size[i]);
		} 
	}
}

// Thread that handles the communication with the test aplications
void * clientThread(void * arg) {
	printf("               clientThread\n");
	thread_info_struct *threadInfo = arg;
	int client = threadInfo->inputArgument;

	Message_struct messageReceived;

	// Communication with the client
	int receivedBytesMessage = 1;
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

			// Informs the other threads on the clipboard three that there is new information on the clipboard
			clipboard.update = 1;
			clipboard.newInfo[0] = threadInfo->thread_id;
			clipboard.newInfo[1] = messageReceived.region;

			/*if(modeOfFunction == 1) {
				if(copyBackup(messageReceived, sock_fd_inet) == 0) {
					printf("Error backing up new data\n");
				}
			}*/
		}
		else if(messageReceived.action == PASTE) {
			//printf("Received information - action: PASTE\n");
			printf("\nPASTE\n");

			if(paste(messageReceived, client) == 0) {
				printf("Error on pasting\n");
			}
		}
	}

	// Free the struct that holds the information about the thread
	free(threadInfo);
	printf("GoodBye\n");
}

void * upThread(void * sock_fd_inetIP) {
	int client = *((int *) sock_fd_inetIP);
	pthread_t thread_id;
	socklen_t size_addr;

	// Connects to the upper clipboard
}

void * clipboardThread(void * threadInput) {
	int receivedBytesMessage = 1;
	int clipboard_id = *((int *) threadInput);

	// Update the info between clipboards
	printf("       clipboardThread\n");

}	

void * downThread(void * arg) {
	thread_info_struct *threadInfo = arg;
	int client = threadInfo->inputArgument;
	pthread_t thread_id;
	socklen_t size_addr;

	while(1) {
		printf(".\n");

		// Reset hold variable
		size_addr = sizeof(struct sockaddr);

		int clipboard_client = accept(client, (struct sockaddr *) &clientClipboard_addr, &size_addr);
		
		printf("Accepted connection\n");
		// Creates new thread to handle the comunicatuion with the client
		//pthread_create(&thread_id, NULL, clipboardThread, &clipboard_client);
		//printf("Thread created clipboard - ID %lu\n",  thread_id);

	}
	free(threadInfo);
}



int main(int argc, char const *argv[]) {
	int statusBackup = 0;
	
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
		if(strcmp(argv[1], ONLINE_FLAG) != 0) {
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

	// Creates the portDown to comunnicate down on the tree
	srand(getpid());   // should only be called once
	int portDown = rand()%(64738-1024) + 1024; 

	printf("Port to acess machine: %d\n", portDown);

	//unlink(SOCKET_ADDR);

	// Create socket unix
	int sock_fd_unix = connect_unix(&local_addr_un);

	// Create socket inet
	// To communicate with down stages 
	sock_fd_inet = connect_inet(&backup_addr, portDown);
	// To communicate with upper stages 
	int sock_fd_inetIP = 0;
	if(modeOfFunction == ONLINE) {
		sock_fd_inetIP = connect_inetIP(&backup_addr, port, ip);
	}

	// Init the clipboard struct
	for (int i = 0; i < 10; i++)
	{
		clipboard.clipboard[i] = NULL;
		clipboard.size[i] = 0;
	}

	// Loads data from Backup
	if(modeOfFunction == ONLINE) {
		backupInit(sock_fd_inetIP);
	}

	// Creates a thread to comunnicate with clipboards down on the tree
	thread_info_struct *threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
	if(threadInfo == NULL) {
		perror("malloc");
		exit(0);
	}

	threadInfo->inputArgument = sock_fd_inet;

	pthread_create(&threadInfo->thread_id, NULL, &downThread, &threadInfo);
	printf("Thread created to handle clipboards down on the tree - ID %lu\n",  threadInfo->thread_id);

	// Creates a thread to communicat with their backup clipboard 
	if(modeOfFunction == ONLINE) {
		thread_info_struct *threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
		if(threadInfo == NULL) {
			perror("malloc");
			exit(0);
		}

		threadInfo->inputArgument = sock_fd_inetIP;
		pthread_create(&threadInfo->thread_id, NULL, &upThread, &threadInfo);
		printf("Thread created to handle clipboard up on the tree - ID %lu\n",  threadInfo->thread_id);
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

		// Creates a structure to hold the information about the thread and the input argument
		threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
		if(threadInfo == NULL) {
			perror("malloc");
			exit(0);
		}

		threadInfo->inputArgument = client;

		printf("Accepted connection\n");
		// Creates new thread to handle the comunicatuion with the client
		pthread_create(&threadInfo->thread_id, NULL, &clientThread, &threadInfo);
		printf("Thread created AT - ID %lu\n",  threadInfo->thread_id);

	}

	exit(0);
}

