#include "clipboardIntern.h"
#include "clipboard.h"

// Variavel que avisa quando é para desligar as threads
int killSignal = 0;

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

// Pipe para comunicação entre threads em local mode
int pipeThread[2];

// POSIX RW LOCK
// Proteger as regioes do clipboard
pthread_rwlock_t rwlockClipboard;



/**
 * Unlinks the sockets when the program stops
 * @param signum sinal do kernel
 */
void ctrl_c_callback_handler(int signum) {
	printf("aught signal Ctr-C\n");
	killSignal = 1;

	// Closes the sockets
	close(sock_fd_unix);

	close(sock_fd_inet);

	close(sock_fd_inetIP);

	close(pipeThread[0]);
	
	close(pipeThread[1]);

	unlink(SOCKET_ADDR);

	// Clears the clipboard
	pthread_rwlock_wrlock(&rwlockClipboard);
	for (int i = 0; i < NUMBEROFPOSITIONS; ++i) {
		free(clipboard[i].data);
	}
	pthread_rwlock_unlock(&rwlockClipboard);
	

	exit(0);
}

/**
 * Gera o número do porto de forma aleatoria a partir do PID do processo
 * @return número do porto
 */
int randomPort() {
	// Creates port to comunnicate with clipboards down on the tree
	srand(getpid());   // seeds the port number on the pid of the process
	int port = rand()%(100) + 8000; 

	return port;
}




/**
 * Coneta-se a um socket unix e realiza o bind ao mesmo
 */
void connectUnix() {
	// Unlink do socket
	unlink(SOCKET_ADDR);

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


/**
 * Coneta a um socket responsavel pelas ligacoes abaixo na arvore, imprime no ecra esse porto
 */
int connect_inet() {
	int port;
	int err = -1;
	
	sock_fd_inet = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd_inet == -1) {
		perror("socket inet");
		exit(-1);
	}

	// Itera vários portos até encontrar um disponivel para fazer bind
	while(err == -1) {
		port = randomPort();
		local_addr_in.sin_family = AF_INET;
		local_addr_in.sin_port= htons(port);
		local_addr_in.sin_addr.s_addr= INADDR_ANY;

		err = bind(sock_fd_inet, (struct sockaddr *) &local_addr_in, sizeof(struct sockaddr_in));
		if(err != -1) {
			// Caso consiga fazer o bind, para o ciclo;
			break;
		}
	}

	if(listen(sock_fd_inet, 2) == -1) {
		perror("listen)");
		exit(-1);
	}

	// Imprime o porto para aceder ao clipboard
	printf("Port to acess machine: %d\n", port);

	return port;
}


/**
 * Coneta a um socket responsavel pelas ligacoes acima na arvore
 * @param port número do porto
 * @param ip   IP do outro computador
 */
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

/**
 * Inicializa um pipe
 */
void createPipe() {
	if(pipe(pipeThread) != 0) {
		perror("pipe");
		exit(-1);
	}
}

void initClipboard() {
	for (int i = 0; i < NUMBEROFPOSITIONS; i++)
	{
		clipboard[i].data = NULL;
		clipboard[i].size = 0;
	}
}



int main(int argc, char const *argv[]) {
	// Atach the ctrl_c_callback_handler to the SIGINT signal
	if(signal(SIGINT, ctrl_c_callback_handler) == SIG_ERR) {
		printf("Falha de sistema - signal handler\n");
		exit(-3);
	}
	// Ignora o signal pipe, é tratado pela aplicação
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		printf("Falha de sistema - signal pipe\n");
		exit(-3);
	}

	int portUp = -1;
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

	// Inicializa o clipboard
	initClipboard();

	// Inicia os RWLOCK
	initRWLock();

	// Inicia os MUTEX
	initMutex();

	// Inicia as Variaveis de confição
	int initCondWait();

	//Ligação aos sockets
	// Coneta-se ao socket de dominio unix
	connectUnix();
	// Coneta-se ao socket de dominio inet para comunicações abaixo na arvore
	connect_inet();
	// Se tiver online, coneta-se a um socket inet ao clipboard acima na arvore
	if(modeOfFunction == ONLINE) {
		connect_inetIP(portUp, ip);
	}

	// Cria um pipe para comunicar em modo local entre threads
	createPipe();

	thread_info_struct *threadInfo = NULL;

	// Inicializa a upThread responsavel por receber as comunicações de cima da arvore
	threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
	if(threadInfo == NULL) {
		perror("malloc");
		exit(-1);
	}

	threadInfo->inputArgument = sock_fd_inetIP;
	pthread_create(&threadInfo->thread_id, NULL, &upThread, threadInfo);

	// Inicializa a downThread responsável por aceitar as conecções de outros a este clipboard
	threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
	if(threadInfo == NULL) {
		perror("malloc");
		exit(-1);
	}

	threadInfo->inputArgument = sock_fd_inet;
	pthread_create(&threadInfo->thread_id, NULL, &downThread, threadInfo);

	printf("Ready to accept clients\n");
	// Aceita aplicacoes que se tentem ligar ao cliboarc
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
		threadInfo->type = APP;

		printf("Accepted connection of app\n");
		// Creates new thread to handle the comunicatuion with the client
		pthread_create(&threadInfo->thread_id, NULL, &clientThread, threadInfo);
	}

	return 0;
}