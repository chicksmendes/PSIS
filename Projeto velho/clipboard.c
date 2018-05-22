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

// sock fd of the upper socket
int upperClipboard; 

// List of active clipboard threads
thread_info_struct *clipboardThreadList = NULL;

// POSIX RW LOCK
pthread_rwlock_t rwlockClipboard;
pthread_rwlock_t rwlockModeOfFunction;

// POSIX MUTEX
pthread_mutex_t ThreadListMutex;
pthread_mutex_t sendMutex;
pthread_mutex_t waitingThreadsMutex;

// POSIX CONDITIONAL VARIABLE
pthread_cond_t condClipboard[NUMBEROFPOSITIONS];
int waitingThreads[NUMBEROFPOSITIONS];

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
	thread_info_struct *aux2 = NULL;
	while(aux != NULL) {
		if(aux->thread_id == thread_id) {
			// Beggining of the list
			if(aux->thread_id == clipboardThreadList->thread_id) {
				clipboardThreadList = clipboardThreadList->next;
				break;
			}
			// End of the list
			else if(aux->next == NULL) {
				aux2->next = NULL;
				break;
			}
			// Midle of the list
			aux2->next = aux->next;
			break; 
		}
		aux2 = aux;
		aux = aux->next;
	}
	free(aux);
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

	if(listen(sock_fd_inet, 2) == -1) {
		perror("listen)");
		exit(-1);
	}
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
        if (sentBytes <= 0 ) { 
        	return sentBytes; 
        }
        total += sentBytes;
        bytesleft -= sentBytes;
    }

    // Returns -1 if cannot send information, returns total bytes sent otherwise
    return total; 
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
        if (receiveBytes <= 0) { 
        	return receiveBytes; 
        }
        total += receiveBytes;
        bytesleft -= receiveBytes;
    }

    // Returns -1 if cannot receive information, returns total bytes receive otherwise
    return total; 
}

/* LOCAL */
int copy(Message_struct messageReceived, int client) {
	// New data received
	char *data = NULL;

	// Allocs memory to store new data
	data = (char *)malloc(sizeof(char)*messageReceived.size[messageReceived.region]);
	if(data == NULL) {
		perror("malloc");
		exit(-1);
	}

	// Receives the data from the client
	if(readAll(client, data, messageReceived.size[messageReceived.region]) != messageReceived.size[messageReceived.region]) {
		perror("copy");
		return -1;
	}

	// Locks the write to the socket/pipe so only can write at once
	pthread_mutex_lock(&sendMutex);
	// Sends the update to the upper clipboard
	pthread_rwlock_rdlock(&rwlockModeOfFunction);
	if(modeOfFunction == ONLINE) {
		if(write(sock_fd_inetIP, &messageReceived, sizeof(Message_struct)) != sizeof(Message_struct)) {
			perror("copy");
			return -1;
		}

		if(writeAll(sock_fd_inetIP, data, messageReceived.size[messageReceived.region]) != messageReceived.size[messageReceived.region]) {
			perror("copy");
			return -1;
		}
	}
	// Sends the update to the upper thread trow the pipe
	else if(modeOfFunction == LOCAL) {
		if(write(pipeThread[1], &messageReceived, sizeof(Message_struct)) != sizeof(Message_struct)) {
			perror("copy");
			return -1;
		}

		if(writeAll(pipeThread[1], data, messageReceived.size[messageReceived.region]) != messageReceived.size[messageReceived.region]) {
			perror("copy");
			return -1;
		}
	}
	pthread_rwlock_unlock(&rwlockModeOfFunction);
	pthread_mutex_unlock(&sendMutex);

	// Clears the memory allocated to receive the message
	free(data);

	return 1;
}


int paste(Message_struct messageReceived, int client) {
	
	int error = 0;
	int success = 1;
	Message_struct messageSend;

	// Critical Region - Read
	pthread_rwlock_rdlock(&rwlockClipboard);
	int size = clipboard.size[messageReceived.region];
	pthread_rwlock_unlock(&rwlockClipboard);

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
		// Critical Region - Read
		pthread_rwlock_rdlock(&rwlockClipboard);
		memcpy(buff, clipboard.clipboard[messageSend.region], size);
		pthread_rwlock_unlock(&rwlockClipboard);


		// Sends the data to the client
		int numberOfBytesPaste = writeAll(client, buff, size);
		if(numberOfBytesPaste != size) {
			perror("paste");
			exit(-1);
		}
		printf("Sent %d bytes - data: %s\n", numberOfBytesPaste, buff);

		free(buff);
	}

	return 1;
}

int wait(Message_struct messageReceived, int client) {

	int size = 0;

	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&mutex);

	pthread_mutex_lock(&waitingThreadsMutex);
	waitingThreads[messageReceived.region]++;
	pthread_mutex_unlock(&waitingThreadsMutex);

	pthread_cond_wait(&condClipboard[messageReceived.region], &mutex);

	pthread_mutex_unlock(&mutex);

	pthread_mutex_lock(&waitingThreadsMutex);
	waitingThreads[messageReceived.region]--;
	pthread_mutex_unlock(&waitingThreadsMutex);

	pthread_rwlock_rdlock(&rwlockClipboard);
	if(clipboard.size[messageReceived.region] > messageReceived.size[messageReceived.region]) {
		write(client, &messageReceived.size[messageReceived.region], sizeof(size_t));
		size = messageReceived.size[messageReceived.region];
	}
	else {
		write(client, &clipboard.size[messageReceived.region], sizeof(size_t));
		size = clipboard.size[messageReceived.region];
	}
	pthread_rwlock_unlock(&rwlockClipboard);	

	char *buff = (char *)malloc(size);
	if(buff == NULL) {
		perror("wait");
		exit(-1);
	}

	pthread_rwlock_wrlock(&rwlockClipboard);
	memcpy(buff, clipboard.clipboard[messageReceived.region], size);
	pthread_rwlock_unlock(&rwlockClipboard);
	
	if(writeAll(client, buff, size) != size) {
		perror("wait");
	}	

	free(buff);

	pthread_mutex_destroy(&mutex);

	return 1;
}


int backupCopy(int client) {
	Message_struct messageClipboard;
	messageClipboard.action = COPY;

	char *data = NULL;
	
	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		// READ LOCK DO CLIPBOARD
		pthread_rwlock_rdlock(&rwlockClipboard);
		messageClipboard.size[i] = clipboard.size[i];
		pthread_rwlock_unlock(&rwlockClipboard);

		if(messageClipboard.size[i] != 0) {
			data = (char *)malloc(messageClipboard.size[i]);
			if(data == NULL) {
				perror("backupCopy");
			}
			pthread_rwlock_rdlock(&rwlockClipboard);
			memcpy(data, clipboard.clipboard[i], messageClipboard.size[i]);
			pthread_rwlock_unlock(&rwlockClipboard);
			
			printf("region %d data %s\n", i, data);
			
			pthread_mutex_lock(&ThreadListMutex);
			write(client, &messageClipboard, sizeof(Message_struct));

			writeAll(client, data, messageClipboard.size[i]);
			pthread_mutex_unlock(&ThreadListMutex);
			free(data);
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

	Message_struct message;

	// Update the info between clipboards
	while(killSignal == 0) {		
		int numberOfBytesReceived = read(client, &message, sizeof(Message_struct));
		if(numberOfBytesReceived <= 0) {
			printf("Clipboard disconected\n");
			break;
		}
		else if(message.action == COPY) {
			if(copy(message, client) == 0) {
				printf("Error on copy\n");
			}
		}
		else if(message.action == PASTE) {
			if(paste(message, client) == 0) {
				printf("Error on pasting\n");
			}
		}
		else if(message.action == WAIT) {
			if(wait(message, client) == 0) {
				printf("Error on wait\n");
			}
		}
	}
	close(client);
	printf("GoodBye - clientThread\n");
}

// Thread responsable to accept clipboards trying to connect to this one
void * downThread(void * arg) {
	thread_info_struct *threadInfo = arg;
	int client = threadInfo->inputArgument;

	socklen_t size_addr;

	size_addr = sizeof(struct sockaddr);
	
	while(killSignal == 0) {
		int clipboard_client = accept(client, (struct sockaddr *) &clientClipboard_addr, &size_addr);
		if(clipboard_client != -1) {
			
			thread_info_struct *threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
			if(threadInfo == NULL) {
				perror("malloc");
				exit(0);
			}

			threadInfo->inputArgument = clipboard_client;

			backupCopy(clipboard_client);

			// Creates new thread to handle the comunicatuion with the client
			pthread_create(&threadInfo->thread_id, NULL, &clientThread, threadInfo);
			pthread_mutex_lock(&ThreadListMutex);
			clipboardThreadListAdd(threadInfo);
			pthread_mutex_unlock(&ThreadListMutex);
			
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

	int receivedBytes = 0;

	while(killSignal == 0) {
		if(modeOfFunction == ONLINE) {
			// Reads the message send by the upper Clipboards
			receivedBytes = readAll(clipboardClient, (char *) &messageClipboard, sizeof(Message_struct));
			// If client sends EOF, terminates connection
			if(receivedBytes == 0) {
				// Critical Region - Write
				pthread_rwlock_wrlock(&rwlockModeOfFunction);
				modeOfFunction = LOCAL;
				pthread_rwlock_unlock(&rwlockModeOfFunction);
				printf("Changed to LOCAL MODE\n");
				continue;
			}
			if(receivedBytes != sizeof(Message_struct)) {
				perror("upThread");
				return NULL;
			}
		}
		else if(modeOfFunction == LOCAL) {
			// Reads the message send by the upper Clipboards
			if(read(pipeThread[0], &messageClipboard, sizeof(Message_struct)) != sizeof(Message_struct)) {
				perror("upThread");
				return NULL;
			}
		}


		if(messageClipboard.action == COPY) {
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
			if(modeOfFunction == ONLINE) {
				// Reads the message send by the upper Clipboards
				receivedBytes = readAll(clipboardClient, data, size);
				// If client sends EOF, terminates connection
				if(receivedBytes <= 0) {
					// Critical Region - Write
					pthread_rwlock_wrlock(&rwlockModeOfFunction);
					modeOfFunction = LOCAL;
					pthread_rwlock_unlock(&rwlockModeOfFunction);
					printf("Changed to LOCAL MODE\n");
					continue;
				}
				else if(receivedBytes != size) {
					perror("upThread");
					return NULL;
				}
			}
			else if(modeOfFunction == LOCAL) {
				// Reads the message send by the upper Clipboards
				if(readAll(pipeThread[0],  data, size) != size) {
					perror("upThread");
					return NULL;
				}
			}

			char *aux = (char *)malloc(size);
			memcpy(aux, data, size);

			// Critical Region - Write
			pthread_rwlock_wrlock(&rwlockClipboard);
			// Erases old data
			if(clipboard.clipboard[messageClipboard.region] != NULL) {
				free(clipboard.clipboard[messageClipboard.region]);
			}

			// Assigns new data to the clipboard
			clipboard.clipboard[messageClipboard.region] = data;
			clipboard.size[messageClipboard.region] = size;
			pthread_rwlock_unlock(&rwlockClipboard);

			pthread_mutex_lock(&waitingThreadsMutex);
			while(waitingThreads[messageClipboard.region] > 0)
			{
				pthread_cond_signal(&condClipboard[messageClipboard.region]);
				waitingThreads[messageClipboard.region]--;
			}
			pthread_mutex_unlock(&waitingThreadsMutex);

			
			//printf("Received %d bytes - data: %s\n", size, clipboard.clipboard[messageClipboard.region]);
			
			// Sends the new info to the down clipboards
			pthread_mutex_lock(&ThreadListMutex);
			thread_info_struct *sendThreads = clipboardThreadList;
			thread_info_struct *auxList = NULL;

			// Run all the clipboards and send the new information to them
			while(sendThreads != NULL) {
				auxList = sendThreads->next;
				if(write(sendThreads->inputArgument, &messageClipboard, sizeof(Message_struct)) != sizeof(Message_struct)) {
					printf("Client disconected\n");
					clipboardThreadListRemove(sendThreads->thread_id);
				}
				else if(writeAll(sendThreads->inputArgument, aux, size) != size) {
					printf("Client disconected\n");
					clipboardThreadListRemove(sendThreads->thread_id);
				}

				// Passes to the next clipboard
				sendThreads = auxList;
			}
			pthread_mutex_unlock(&ThreadListMutex);
			free(aux);
		}
	}

	close(clipboardClient);
	free(threadInfo);

	printf("GoodBye - upThread\n");
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
	int portDown = rand()%(100) + 8000; 

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
		//backupCopy();
	}

	// Init the clipboard rwlock for the Clipboard
	if(pthread_rwlock_init(&rwlockClipboard, NULL) != 0) {
		perror("rw_lock");
		exit(-1);
	}

	// Init the clipboard rwlock for the mode of function
	if(pthread_rwlock_init(&rwlockModeOfFunction, NULL) != 0) {
		perror("rw_lock");
		exit(-1);
	}

	// Init the clipboard rwlock for the Thread List
	if(pthread_mutex_init(&ThreadListMutex, NULL) != 0) {
		perror("rw_lock");
		exit(-1);
	}
	
	// Init the clipboard rwlock for the Thread List
	if(pthread_mutex_init(&waitingThreadsMutex, NULL) != 0) {
		perror("mutex");
		exit(-1);
	}
	
	// Init the clipboard rwlock for the Clipboard
	if(pthread_mutex_init(&sendMutex, NULL) != 0) {
		perror("mutex");
		exit(-1);
	}

	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		// Init the clipboard rwlock for the Clipboard
		if(pthread_cond_init(&condClipboard[i], NULL) != 0) {
			perror("conditional variable");
			exit(-1);
		}
		waitingThreads[i] = 0;	
	}

	threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
	if(threadInfo == NULL) {
		perror("malloc");
		exit(0);
	}

	threadInfo->inputArgument = sock_fd_inetIP;
	pthread_create(&threadInfo->thread_id, NULL, &upThread, threadInfo);

	// Creates pipe for inter thread communication
	createPipe();


	// Creates a thread to comunnicate with clipboards down on the tree
	threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
	if(threadInfo == NULL) {
		perror("malloc");
		exit(0);
	}

	threadInfo->inputArgument = sock_fd_inet;
	pthread_create(&threadInfo->thread_id, NULL, &downThread, threadInfo);


	printf("Ready to accept clients\n");

	while(1){
		// Reset hold variable
		socklen_t size_addr = sizeof(struct sockaddr);

		// Accept client to communicate
		int client =  accept(sock_fd_unix, (struct sockaddr *) &client_addr_un, &size_addr);
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

	}
	return 0;
}
