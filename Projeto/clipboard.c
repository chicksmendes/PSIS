#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "clipboard.h"
//#include "action.h"

// Sets the mode of utilization - LOCAL/ONLINE
int modeOfFunction;

// Clipboard data
clipboard_struct clipboard;

// Socket structs
// UNIX
struct sockaddr_un local_addr_un;
struct sockaddr_un client_addr_un;

// INET
struct sockaddr_in local_addr_in;
struct sockaddr_in upperClipboard_addr;
struct sockaddr_in clientClipboard_addr;

// Sockets
int sock_fd_unix;
int sock_fd_inet;
int sock_fd_inetIP;

// Kill singnal
int killSignal = 0;

// List of active clipboard threads
thread_info_struct *clipboardThreadList = NULL;

// POSIX Mutexes
pthread_mutex_t muxClipboard;

// pipe for inter thread communication
int pipeThread[2];


// Unlinks the sockets when the program stops
void ctrl_c_callback_handler(int signum){
	printf("aught signal Ctr-C\n");
	// Closes the sockets
	close(sock_fd_inet);

	close(sock_fd_inetIP);

	close(sock_fd_unix);
	// É uma região critia - A TRATAR!!!!
	killSignal = 1;
	// Clears the clipboard
	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		free(clipboard.clipboard[i]);
	}
	
	unlink(SOCKET_ADDR);
	exit(0);
}







/***************************************
 * clipboardThreadList Functions
 ***************************************/


void clipboardThreadListAdd(thread_info_struct *new) {
	if(clipboardThreadList == NULL) {
		new->next = NULL;
		clipboardThreadList = new;
	}
	else {
		new->next = clipboardThreadList;
		clipboardThreadList = new;
	}
}

void clipboardThreadListRemove(pthread_t thread_id) {
	thread_info_struct *aux = clipboardThreadList;
	while(aux != NULL) {
		if(aux->thread_id == thread_id) {
			break;
		}
		aux = aux->next;
	}
	/*****************
	IMPLEMENTAR O RETIRAR DAS THREADS
	**********************/
	
}

/**************************
 * pipe Functions
 **************************/
void createPipe() {
	if(pipe(pipeThread) != 0) {
		perror("pipe");
		exit(-1);
	}
}

/***********************
 * Socket Functions
 ***********************/
/**
 * @brief      Connects an unix socket.
 */
void connect_unix() {
	// Create socket unix
	sock_fd_unix = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock_fd_unix == -1) {
		perror("socket unix");
		exit(-1);
	}

	local_addr_un.sun_family = AF_UNIX;
	strcpy(local_addr_un.sun_path, SOCKET_ADDR);

	// Bind the socket with the address assignet to it
	int err_unix = bind(sock_fd_unix, (struct sockaddr *) &local_addr_un, sizeof(struct sockaddr_un));
	if(err_unix == -1) {
		perror("bind");
		exit(-1);
	}

	// Listen
	err_unix = listen(sock_fd_unix, 5);
	if(err_unix == -1) {
		perror("listen");
		exit(-1);
	}

	//printf("Local socket initiated\n");
}
// Connects to socket responsable with down connections on tree
void connect_inet(int portDown) {
	sock_fd_inet = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd_inet == -1) {
		perror("socket inet");
		exit(-1);
	}

	local_addr_in.sin_family = AF_INET;
	local_addr_in.sin_port= htons(portDown);
	local_addr_in.sin_addr.s_addr= INADDR_ANY;
	int err = bind(sock_fd_inet, (struct sockaddr *) &local_addr_in, sizeof(struct sockaddr_in));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}
	//printf("Socket created and binded\n");

	if(listen(sock_fd_inet, 2) == -1) {
		perror("listen)");
		exit(-1);
	}

	//printf("Ready to accept connections\n");
}

void connect_inetIP(int port, char ip[]) {
	sock_fd_inetIP = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd_inetIP == -1) {
		perror("socket");
		exit(-1);
	}

	upperClipboard_addr.sin_family = AF_INET;
	upperClipboard_addr.sin_port= htons(port);
	inet_aton(ip, &upperClipboard_addr.sin_addr);

	if( -1 == connect(sock_fd_inetIP, (const struct sockaddr *) &upperClipboard_addr, sizeof(struct sockaddr_in))) {
			printf("Error connecting to backup server\n");
			exit(-1);
	}
	//printf("Online socket initiated\n");
}





/***********************
 * Clipboard Functions
 ***********************/
/**
 * @brief      Attemps to write a buffer and if the write fails,
 *             writes again the rest of the buffer 
 *
 * @param[in]  sock_fd  The file descriptor of the socket
 * @param      buf      The buffer
 * @param[in]  len      The length of the buffer
 *
 * @return     Returns the number of bytes writen or -1 in case of fail
 */
int writeAll(int sock_fd, char *buf, int len) {
	// Number of bytes sent
	int total = 0;
	// Number of bytes left to send
    int bytesleft = len;
    int sentBytes;

    while(total < len) {
        sentBytes = write(sock_fd, buf+total, bytesleft);
        if (sentBytes == -1) { 
        	break; 
        }
        total += sentBytes;
        bytesleft -= sentBytes;
    }

    // Returns -1 if cannot send information, returns total bytes sent otherwise
    return sentBytes == -1?-1:total; 
}

/**
 * @brief      Attemps to read a buffer and if the read fails,
 *             writes again the rest of the buffer 
 *
 * @param[in]  sock_fd  The file descriptor of the socket
 * @param      buf      The buffer
 * @param[in]  len      The length of the buffer
 *
 * @return     Returns the number of bytes read or -1 in case of fail
 */
int readAll(int sock_fd, char *buf, int len) {
	// Number of bytes received
	int total = 0;
	// Number of bytes left to receive
    int bytesleft = len;
    int receiveBytes;

    while(total < len) {
        receiveBytes = read(sock_fd, buf+total, bytesleft);
        if (receiveBytes == -1) { 
        	break; 
        }
        total += receiveBytes;
        bytesleft -= receiveBytes;
    }

    // Returns -1 if cannot receive information, returns total bytes receive otherwise
    return receiveBytes == -1?-1:total; 
}

/* LOCAL */
int copy(Message_struct messageReceived, int client) {
	
	//int error = 0;
	//int success = 1;

	// New data received
	char *data = NULL;

	// Allocs memory to store new data
	data = (char *)malloc(sizeof(char)*messageReceived.size[messageReceived.region]);
	if(data == NULL) {
		perror("malloc");
		exit(-1);
	}
	// Store the size of the clipboard region
	int size = messageReceived.size[messageReceived.region];

	// Receives the data from the client
	//int numberOfBytesCopied = re
	int numberOfBytesCopied = readAll(client, data, size);

	// REGIAO CRITICA 
	pthread_mutex_lock(&muxClipboard);
	// Erases old data
	if(clipboard.clipboard[messageReceived.region] != NULL) {
		printf("Region cleared\n");
		free(clipboard.clipboard[messageReceived.region]);
	}

	// Assigns new data to the clipboard
	clipboard.size[messageReceived.region] = size;
	clipboard.clipboard[messageReceived.region] = data;
	pthread_mutex_unlock(&muxClipboard);

	printf("Received %d bytes - data: %s\n", numberOfBytesCopied, clipboard.clipboard[messageReceived.region]);

	// Sends the update to the transmissionThread
	updateMessage update;
	update.region = messageReceived.region;
	update.source = DOWN;
	if(write(pipeThread[1], &update, sizeof(updateMessage)) != sizeof(updateMessage)) {
		perror("pipe write");
		exit(-1);
	}
	//printf("Write to pipe %d %d\n", update.region, update.source);


	return 1;
}


int paste(Message_struct messageReceived, int client) {
	
	int error = 0;
	int success = 1;
	Message_struct messageSend;

	//REGIAO CRITICA
	pthread_mutex_lock(&muxClipboard);
	int size = clipboard.size[messageReceived.region];
	pthread_mutex_unlock(&muxClipboard);

	if(messageReceived.size[messageReceived.region] < size) {
		if(write(client, &error, sizeof(int)) != sizeof(int)) {
			perror("paste");
			exit(-1);
		}
	}
	else {
		if(write(client, &success, sizeof(int)) != sizeof(int)) {
			perror("paste");
			exit(-1);
		}

		// Loads the structure with the information to the client
		messageSend.action = PASTE;
		messageSend.size[messageReceived.region] = size;
		messageSend.region = messageReceived.region;

		write(client, &messageSend, sizeof(Message_struct));

		char *buff = (char*)malloc(size);
		if(buff == NULL) {
			perror("malloc");
			exit(-1);
		}
		//	REGIAO CRITICA
		pthread_mutex_lock(&muxClipboard);
		memcpy(buff, clipboard.clipboard[messageSend.region], size);
		pthread_mutex_unlock(&muxClipboard);

		printf("transmissionThread - buffer %s\n", buff);

		// Sends the data to the client
		int numberOfBytesPaste = writeAll(client, buff, size);
		if(numberOfBytesPaste != size) {
			perror("paste");
			exit(-1);
		}
		printf("Sent %d bytes - data: %s\n", numberOfBytesPaste, buff);
	}

	return 1;
}


int backupPaste(Message_struct messageClipboard, int clipboard_client) {

	//REGIAO CRITICA 
	pthread_mutex_lock(&muxClipboard);
	for (int i = 0; i < NUMBEROFPOSITIONS; ++i) {
		messageClipboard.size[i] = clipboard.size[i];
	}
	pthread_mutex_unlock(&muxClipboard);

	// Sends the amount of data present in the clipboard
	write(clipboard_client, &messageClipboard, sizeof(Message_struct));

	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		if(messageClipboard.size[i] != 0) {
			char *buff = (char*)malloc(messageClipboard.size[i]);
			if(buff == NULL) {
				perror("malloc");
				exit(-1);
			}
			//REGIAO CRITICA
			pthread_mutex_lock(&muxClipboard);
			memcpy(buff, clipboard.clipboard[i], messageClipboard.size[i]);
			pthread_mutex_unlock(&muxClipboard);

			printf("transmissionThread - buffer %s\n", buff);

			printf("region %d ", i);
			printf("clipboard content %s size %d\n", clipboard.clipboard[i], (int ) clipboard.size[i]);
			if(writeAll(clipboard_client, buff, messageClipboard.size[i]) != messageClipboard.size[i]) {
				perror("backup paste");
				exit(-1);
			}
		} 
	}
	printf("backupPaste complete\n");

	return 1;
}


int backupCopy() {
	Message_struct messageClipboard;
	messageClipboard.action = BACKUP;

	int statusBackup = write(sock_fd_inetIP, &messageClipboard, sizeof(Message_struct));
	if(statusBackup == 0) {
		printf("cannot acess backup\n");
	}

	if(read(sock_fd_inetIP, &messageClipboard, sizeof(Message_struct)) != sizeof(Message_struct)) {
		perror("Commucation");
	}

	printf("Performing backup\n");
	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		if(messageClipboard.size[i] != 0) {
			// New data received
			char *data = NULL;
			printf("region %d ", i);
			data = (char *)malloc(sizeof(char)*messageClipboard.size[i]);
			if(data == NULL) {
				perror("malloc");
				exit(0);
			}

			// Store the size of the clipboard region
			clipboard.size[i] = messageClipboard.size[i];
			int numberOfBytesBackup = readAll(sock_fd_inetIP, data, clipboard.size[i]);
			if(numberOfBytesBackup != clipboard.size[i]) {
				printf("Number of bytes received backup is Incorrect. Received %d and it should be %d\n", numberOfBytesBackup, (int ) clipboard.size[i]);
				break;
			}
			clipboard.clipboard[i] = data;
			printf("	received %s, size %d\n", clipboard.clipboard[i], numberOfBytesBackup);
		}
	}

	printf("backupCopy complete\n");

	return 1;
}






/***********************
 * Thread Functions
 ***********************/
// Thread responsable to communicate with AP_TEST
void * clientThread(void * arg) {
	thread_info_struct *threadInfo = arg;
	int client = threadInfo->inputArgument;
	printf("Thread %lu Client %d\n", threadInfo->thread_id, threadInfo->inputArgument);
	int receivedBytes;

	Message_struct messageReceived;
	printf("Client Thread\n");
	while(killSignal == 0){
printf(". clientThread\n");
		// Reads the inbood message from the client
		receivedBytes = read(client, &messageReceived, sizeof(Message_struct));

		// If client sends EOF, terminates connection
		if(receivedBytes == 0) {
			break;
		}

		//printf("Client Thread - message receive\n");
		if(messageReceived.action == COPY) {
			printf("\nCOPY\n");
			if(copy(messageReceived, client) == 0) {
				printf("Error on copy\n");
				break;
			}
		}
		else if(messageReceived.action == PASTE) {
			//printf("Received information - action: PASTE\n");
			printf("\nPASTE\n");

			if(paste(messageReceived, client) == 0) {
				printf("Error on pasting\n");
				break;
			}
		}
	}
	close(client);
	// Free the struct attach to the thread
	free(threadInfo);
	printf("GoodBye - clientThread\n");
}


// Thread responsable to communicate with down Clipboards
void * clipboardThread(void * arg) {
	thread_info_struct *threadInfo = arg;
	int clipboard_client = threadInfo->inputArgument;

	Message_struct messageClipboard;

	// Update the info between clipboards
	while(killSignal == 0) {
printf(".       clipboardThread\n");
		
		int numberOfBytesReceived = read(clipboard_client, &messageClipboard, sizeof(Message_struct));

printf(".       clipboardThread - action %d\n", messageClipboard.action);
		
		if(numberOfBytesReceived == 0) {
			printf("Clipboard disconected\n");
			break;
		}
		if(messageClipboard.action == BACKUP) {
			printf("BACKUP\n");
			if(backupPaste(messageClipboard, clipboard_client) == 0) {
				printf("Error on backup\n");
			}
		}
		/*else if(messageClipboard.action == PASTE) {
			printf("PASTE\n");
			if(paste(messageClipboard, clipboard_client) == 0) {
				printf("Error on copy\n");
			}
		}*/
		else if(messageClipboard.action == COPY) {
			printf("\nCOPY\n");
			if(copy(messageClipboard, clipboard_client) == 0) {
				printf("Error on copy\n");
			}
		}
	}
	close(clipboard_client);
	// Free the struct attach to the thread
	free(threadInfo);
	printf("GoodBye - clipboardThread\n");
}


// Thread responsable to accept clipboards trying to connect to this one
void * downThread(void * arg) {
	thread_info_struct *threadInfo = arg;
	int client = threadInfo->inputArgument;

	socklen_t size_addr;

	size_addr = sizeof(struct sockaddr);
	
	while(killSignal == 0) {
printf(". downThread\n");
		int clipboard_client = accept(client, (struct sockaddr *) &clientClipboard_addr, &size_addr);
		if(clipboard_client != -1) {

			thread_info_struct *threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
			if(threadInfo == NULL) {
				perror("malloc");
				exit(0);
			}

			threadInfo->inputArgument = clipboard_client;
			// Creates new thread to handle the comunicatuion with the client
			pthread_create(&threadInfo->thread_id, NULL, &clipboardThread, threadInfo);
			clipboardThreadListAdd(threadInfo);
			printf("Thread created clipboard - ID %lu\n",  threadInfo->thread_id);

			size_addr = sizeof(struct sockaddr);			
		}

	}
	free(threadInfo);
	printf("GoodBye - downThread\n");
}


// Thread respnsable to communcate with upeer Clipboards
void * upThread(void *arg) {
	thread_info_struct *threadInfo = arg;
	int clipboardClient = threadInfo->inputArgument;

	Message_struct messageClipboard;

	while(killSignal == 0) {
printf(". upThread\n");
		// Reads the message send by the upper Clipboards
		if(read(clipboardClient, &messageClipboard, sizeof(Message_struct)) != sizeof(Message_struct)) {
			perror("upThread");
			break;
		}

		if(messageClipboard.action == COPY) {
			//int error = 0;
			//int success = 1;

			// New data received
			char *data = NULL;

			// Allocs memory to store new data
			data = (char *)malloc(sizeof(char)*messageClipboard.size[messageClipboard.region]);
			if(data == NULL) {
				perror("malloc");
				exit(-1);
			}

			// Store the size of the clipboard region
			int size = messageClipboard.size[messageClipboard.region];

			// Receives the data from the client
			//int numberOfBytesCopied = re
			int numberOfBytesCopied = readAll(clipboardClient, data, size);

			//int numberOfBytesCopied = read(client, data, clipboard.size[messageReceived.region]);

			// REGIAO CRÍTICA
			pthread_mutex_lock(&muxClipboard);
			// Erases old data
			if(clipboard.clipboard[messageClipboard.region] != NULL) {
				printf("Region cleared\n");
				free(clipboard.clipboard[messageClipboard.region]);
			}

			// Assigns new data to the clipboard
			clipboard.clipboard[messageClipboard.region] = data;
			clipboard.size[messageClipboard.region] = size;
			pthread_mutex_unlock(&muxClipboard);

			printf("Received %d bytes - data: %s\n", numberOfBytesCopied, clipboard.clipboard[messageClipboard.region]);

			updateMessage update;
			update.region = messageClipboard.region;
			update.source = UP;
			// Transmit the information to the transmission thread to update lower clients
			if(write(pipeThread[1], &update, sizeof(updateMessage)) != sizeof(updateMessage)) {
				perror("pipe write");
				exit(-1);
			}

			printf("Write to pipe %d %d\n", update.region, update.source);
		}
	}

	close(clipboardClient);
	free(threadInfo);

	// If the connection to the upper clipboard is lost, start to work like a local clipboard
	modeOfFunction = LOCAL;
	printf("Mode of function - LOCAL\n");	
	printf("GoodBye - upThread\n");
}

// Thread respnsable to transmit information to the others Clipboards
void * transmissionThread(void *arg) {
	thread_info_struct *threadInfo = arg;
	int clipboardClient = threadInfo->inputArgument;

	Message_struct messageClipboard;

	updateMessage update;

	while(killSignal == 0) {
//printf("transmissionThread - transmit\n");
		if(read(pipeThread[0], &update, sizeof(updateMessage)) == sizeof(updateMessage)) {

			//printf("read from pipe %d %d\n", update.region, update.source);

			// Critical Region
			pthread_mutex_lock(&muxClipboard);
			char *buff = (char*)malloc(clipboard.size[update.region]);
			if(buff == NULL) {
				perror("malloc");
				exit(-1);
			}
			int size = clipboard.size[update.region];
			memcpy(buff, clipboard.clipboard[update.region], size);
			pthread_mutex_unlock(&muxClipboard);

			// Updates the upper clipboards with the new info
			messageClipboard.region = update.region;
			messageClipboard.action = COPY;
			messageClipboard.size[update.region] = size;
			
			if(modeOfFunction == ONLINE && update.source == DOWN) {
				//printf("transmissionThread - online e baixo\n");
				if(write(clipboardClient, &messageClipboard, sizeof(Message_struct)) != sizeof(Message_struct)) {
					perror("transmissionThread - up");
					exit(-1);
				}
				else {
					writeAll(clipboardClient, buff, size);
				}
			}

			else if((modeOfFunction == ONLINE && update.source == UP) || (modeOfFunction == LOCAL && update.source == DOWN)) {
				// Update the down clipboards with the new info
				thread_info_struct *sendThreads = clipboardThreadList;

				// Run all the clipboards and send the new information to them
				while(sendThreads != NULL) {
					//printf("transmissionThread - baixo\n");
					if(write(sendThreads->inputArgument, &messageClipboard, sizeof(Message_struct)) != sizeof(Message_struct)) {
						perror("transmissionThread - down");
						exit(-1);
					}
					
					if(writeAll(sendThreads->inputArgument, buff, size) != size) {
						perror("transmissionThread - down");
						exit(-1);
					}

					// Next clipboard connected
					sendThreads = sendThreads->next;
				}				
			}

			free(buff);
		}
		else {
			perror("pipe read");
			exit(-1);
		}
	}

	close(clipboardClient);
	free(threadInfo);
	printf("GoodBye - transmissionThread\n");	
}






int main(int argc, char const *argv[])
{
	// Atach the ctrl_c_callback_handler to the SIGINT signal
	signal(SIGINT, ctrl_c_callback_handler);

	int portUp;
	char ip[14];

	if(argc == 1) {
		printf("Only local host\n");
		modeOfFunction = LOCAL;
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
		portUp = atoi(argv[3]);
	}
	else {
		printf("Incorrect initialization - number of arguments\n");
		exit(0);
	}

	// Creates port to comunnicate with clipboards down on the tree
	srand(getpid());   // seeds the port number on the pid of the process
	int portDown = rand()%(64738-1024) + 1024; 

	// Informs the user of what is the port to connect
	printf("Port to acess machine: %d\n", portDown);


unlink(SOCKET_ADDR);

	// Init the clipboard struct
	for (int i = 0; i < 10; i++)
	{
		clipboard.clipboard[i] = NULL;
		clipboard.size[i] = 0;
	}

	// Creates the unix socket to communicate with local apps
	connect_unix();

	// Create socket inet
	// To communicate with down stages 
	connect_inet(portDown);

	thread_info_struct *threadInfo = NULL;
	
	// Creates a thread to comunnicate with clipboard up on the tree
	if(modeOfFunction == ONLINE) {
		connect_inetIP(portUp, ip);

		// Receives the backup from the other clipboard
		backupCopy();

		printf("Received backup\n");

		threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
		if(threadInfo == NULL) {
			perror("malloc");
			exit(0);
		}

		threadInfo->inputArgument = sock_fd_inetIP;
		pthread_create(&threadInfo->thread_id, NULL, &upThread, threadInfo);
		//printf("Thread created to handle clipboards up on the tree - ID %lu\n",  threadInfo->thread_id);
	}

	// Creates pipe for inter thread communication
	createPipe();

	// Init thr clipboard mutex
	if(pthread_mutex_init(&muxClipboard, NULL) != 0) {
		perror("Init Mutex");
		exit(-1);
	}

	threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
	if(threadInfo == NULL) {
		perror("malloc");
		exit(0);
	}

	threadInfo->inputArgument = sock_fd_inetIP;
	pthread_create(&threadInfo->thread_id, NULL, &transmissionThread, threadInfo);
	//printf("Thread created to transmit information to other clipboards - ID %lu\n",  threadInfo->thread_id);

	// Creates a thread to comunnicate with clipboards down on the tree
	threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
	if(threadInfo == NULL) {
		perror("malloc");
		exit(0);
	}

	threadInfo->inputArgument = sock_fd_inet;
	pthread_create(&threadInfo->thread_id, NULL, &downThread, threadInfo);
	//printf("Thread created to handle clipboards down on the tree - ID %lu\n",  threadInfo->thread_id);


	printf("Ready to accept clients\n");

	while(1){
//printf(". main\n");

		// Reset hold variable
		socklen_t size_addr = sizeof(struct sockaddr);

		// Accept client to communicate
		int client =  accept(sock_fd_unix, (struct sockaddr *) &client_addr_un, &size_addr);
		printf("client %d\n", client);
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
		pthread_create(&threadInfo->thread_id, NULL, &clientThread, threadInfo);
		printf("Thread created AT - ID %lu\n",  threadInfo->thread_id);

	}
	return 0;
}
