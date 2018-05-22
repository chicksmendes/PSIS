#include "clipboardIntern.h"

// Regioes do clipboard
clipboard_struct clipboard[NUMBEROFPOSITIONS];

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

// Indica se o clipboard esta em modo local ou online
int modeOfFunction;


/**
 * Unlinks the sockets when the program stops
 * @param signum sinal do kernel
 */
void ctrl_c_callback_handler(int signum) {
	printf("aught signal Ctr-C\n");
	close(sock_fd_unix);
	// Closes the sockets
	/*close(sock_fd_inet);

	close(sock_fd_inetIP);

	// É uma região critia - A TRATAR!!!!
	killSignal = 1;
	// Clears the clipboard
	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		free(clipboard.clipboard[i]);
	}
	
	unlink(SOCKET_ADDR);*/
	exit(0);
}

/**
 * Gera o número do porto de forma aleatoria a partir do PID do processo
 * @return número do porto
 */
int randomPort() {
	// Creates port to comunnicate with clipboards down on the tree
	srand(getpid());   // seeds the port number on the pid of the process
	int portDown = rand()%(100) + 8000; 

	return portDown;
}

/**
 * Coneta-se a um socket unix
 */
void connectUnix() {
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



int main(int argc, char const *argv[]) {
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

	int portDown = randomPort();

	// Imprime o porto para aceder ao clipboard
	printf("Port to acess machine: %d\n", portDown);

	// Inicializa o clipboard
	for (int i = 0; i < 10; i++)
	{
		clipboard[i].data = NULL;
		clipboard[i].size = 0;
	}

	// Coneta-se ao socket de dominio unix
	connectUnix();

	thread_info_struct *threadInfo = NULL;

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