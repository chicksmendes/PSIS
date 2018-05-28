#include "threads.h"

// Variavel que avisa quando é para desligar as threads
extern int killSignal;

// Regioes do clipboard
extern clipboard_struct clipboard[NUMBEROFPOSITIONS];

// Sockets
extern int sock_fd_unix;
extern int sock_fd_inet;
extern int sock_fd_inetIP;

// Sockets estruturas
extern struct sockaddr_in clientClipboard_addr;

// Indica se o clipboard esta em modo local ou online
extern int modeOfFunction;
// Pipe para comunicação entre threads em local mode
extern int pipeThread[2];
// Lista de clipboards ligados a este
thread_info_struct *clipboardThreadList = NULL;

// POSIX RW LOCK
// Proteger as regioes do clipboard
extern pthread_rwlock_t rwlockClipboard;
// Proteger a mudanca do modo de funcionamento
pthread_rwlock_t rwlockModeOfFunction;


// POSIX MUTEX
// Proteger a escrita e leitura da lista de threads
pthread_mutex_t threadListMutex;
// Proteger que vários escrevam para o clipboard/pipe para transmitir a informação
pthread_mutex_t sendMutex;

pthread_mutex_t waitingThreadsMutex;

// POSIX CONDITIONAL VARIABLE
pthread_cond_t condClipboard[NUMBEROFPOSITIONS];
// Numero de threads bloqueadas no wait por região
int waitingThreads[NUMBEROFPOSITIONS];


/**
 * Inicializa os mutex do sistema
 * @return 1 em caso de sucesso
 */
int initMutex() {
	// Inicia o mutex para a lista de threads (clipboards)
	if(pthread_mutex_init(&threadListMutex, NULL) != 0) {
		perror("rw_lock");
		exit(-1);
	}
	// Inicia o mutex que impede que varios escrevam no socket superior
	if(pthread_mutex_init(&sendMutex, NULL) != 0) {
		perror("rw_lock");
		exit(-1);
	}

	// Inicia o mutex que impede que varios escrevam no socket superior
	if(pthread_mutex_init(&waitingThreadsMutex, NULL) != 0) {
		perror("rw_lock");
		exit(-1);
	}

	return 1;
}

/**
 * Inicializa os rwLocks do sistema
 * @return [description]
 */
int initRWLock() {
	// Inicia o rwlock para as regioes do Clipboard
	if(pthread_rwlock_init(&rwlockClipboard, NULL) != 0) {
		perror("rw_lock");
		exit(-1);
	}

	// Inicia o rwlock para o modo de funcionamento
	if(pthread_rwlock_init(&rwlockModeOfFunction, NULL) != 0) {
		perror("rw_lock");
		exit(-1);
	}

	return 1;
}

/**
 * Inicializa os conditional waits do sistema
 * @return [description]
 */
int initCondWait() {
	// Inicia as variaveis de condição para as regioes do Clipboard
	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		// Init the clipboard rwlock for the Clipboard
		if(pthread_cond_init(&condClipboard[i], NULL) != 0) {
			perror("conditional variable");
			exit(-1);
		}
		waitingThreads[i] = 0;	
	}

	return 1;
}



/**
 * Adiciona uma estrutura de thread à lista de Threads
 * @param new estrutura da nova thread
 */
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

/**
 * Procura uma thread na lista de threads e remove-a da lista
 * @param thread_id número da thread
 */
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

/**
 * @brief      Escreve dados para um socket ou pipe, em caso de não ter conseguido 
 * enviar tudo, tenta novamente até enviar tudo
 *
 * @param[in]  sock_fd  O file descriptor do socket
 * @param      buf      Buffer para guardar informação
 * @param[in]  len      A quuatidade de informação que se pretende escrever
 *
 * @return     Retorna o núumero de bytes lidos ou -1 em caso de perda de conecção
 */
int writeAll(int sock_fd, char *buf, int len) {
	// Number of bytes sent
	int total = 0;
	// Number of bytes left to send
    int bytesleft = len;
    int sentBytes;

    while(total < len) {
        sentBytes = write(sock_fd, buf+total, bytesleft);
        if(sentBytes <= 0) { 
        	return -1; 
        }
        total += sentBytes;
        bytesleft -= sentBytes;
    }
    return total; 
}

/**
 * @brief      Le dados para um socket ou pipe, em caso de não ter conseguido 
 * enviar tudo, tenta novamente até ler tudo
 *
 * @param[in]  sock_fd  o file descriptor do cliente
 * @param      buf      Buffer para guardar informação
 * @param[in]  len      Número de bytes que se prentende ler
 *
 * @return     Retorna o núumero de bytes lidos ou -1 em caso de perda de conecção
 */
int readAll(int sock_fd, char *buf, int len) {
	// Number of bytes received
	int total = 0;
	// Number of bytes left to receive
    int bytesleft = len;
    int receiveBytes;

    while(total < len) {
        receiveBytes = read(sock_fd, buf+total, bytesleft);
        if(receiveBytes <= 0) { 
        	return -1; 
        }
        total += receiveBytes;
        bytesleft -= receiveBytes;
    }

    // Returns -1 if cannot receive information, returns total bytes receive otherwise
    return total; 
}



/**
 * Realiza um copy - recebe a mensagem de um cliente e envia para esta ser propagada
 * @param  messageReceived messagem com os dados sobre a nova informacao
 * @param  client          file descriptor do cliente
 * @param  type            refere a origem da mensagem - APP ou CLIPBOARD
 * @return                 1 sucesso, -1 em caso de perda de conecção com o cliente
 */
int copy(Message_struct messageReceived, int client, int type) {
	// New data received
	char *data = NULL;

	int success = 1;
	int error = 0;

	// Aloca memória para guardar a nova informação
	data = (char *)malloc(sizeof(char)*messageReceived.size);
	if(data == NULL) {
		printf("Erro a alocar memoria do cliente\n");
		if(type == APP) {
			// Informa a aplicacao que nao conseguiu alocar espaço
			if(write(client, &error, sizeof(int)) != sizeof(int)) {
				return -1;
			}
			return -1;
		}
		else {
			exit(-1);
		}
	}
	// Informa aplicacao que esta pronto a receber a nova informação
	if(type == APP) {
		if(write(client, &success, sizeof(int)) != sizeof(int)) {
			return -1;
		}
	}

	// Recebe a data do cliente
	if(readAll(client, data, messageReceived.size) != messageReceived.size) {
		return -1;
	}

	// Envia a nova informação para o clipboard que esteja em cima deste
	pthread_mutex_lock(&sendMutex);
	if(writeUp(messageReceived, data) == -1) {
		return -1;
	}
	pthread_mutex_unlock(&sendMutex);

	// Clears the memory allocated to receive the message
	free(data);

	return 1;
}

/**
 * Realiza um paste - recebe a mensagem de um cliente com uma regiao pretendida e envia-a
 * @param  messageReceived messagem com os dados sobre a nova informacao
 * @param  client          file descriptor do cliente
 * @param  type            refere a origem da mensagem - APP ou CLIPBOARD
 * @return                 1 sucesso, -1 em caso de perda de conecção
 */
int paste(Message_struct messageReceived, int client, int type) {
	int error = 0;
	int success = 1;

	// Região critica clipboard - leitura
	pthread_rwlock_rdlock(&rwlockClipboard);
	int size = clipboard[messageReceived.region].size;

	// Aloca espaço para um buffer temporário
	char *buff = (char*)malloc(size);
	if(buff == NULL) {
		perror("malloc");
		exit(-1);
	}
	// Copia o conteúdo do clipboard para um buffer temporario
	memcpy(buff, clipboard[messageReceived.region].data, size);
	pthread_rwlock_unlock(&rwlockClipboard);

	// Caso seja uma aplicacao, confirma se esta consegue receber o conteudo do clipboard
	if(type == APP) {
		if(messageReceived.size < size) {
			if(write(client, &error, sizeof(int)) != sizeof(int)) {
				return(-1);
			}
			// Caso o cliente nao tenha espaco suficiente, para a transmissao
			return(-1);
		}
		else {
			if(write(client, &success, sizeof(int)) != sizeof(int)) {
				return(-1);
			}
		}
	}
	

	// Carrega a mensagem com o tamanho da informacao contida no clipboard
	Message_struct messageSend;
	messageSend.action = PASTE;
	messageSend.size = size;
	messageSend.region = messageReceived.region;

	if(write(client, &messageSend, sizeof(Message_struct)) != sizeof(Message_struct)) {
		if(type == APP) {
			return(-1);
		}
		else if(type == CLIPBOARD) {
			exit(-2);
		}
	}

	// Sends the data to the client
	int numberOfBytesPaste = writeAll(client, buff, size);
	if(numberOfBytesPaste != size) {
		if(type == APP) {
			return(-1);
		}
		else if(type == CLIPBOARD) {
			exit(-2);
		}
	}

	// Liberta o buffer temporario
	free(buff);
	
	return 1;
}

/**
 * Espera por uma nova infrmação aparecer numa região do clipboard e envia para o cliente
 * @param  messageReceived mensagem com os parametros da acao pretendida
 * @param  client          file descriptor do cliente
 * @return                 1 em caso de sucesso, -1 em caso de perda de conecção
 */
int wait(Message_struct messageReceived, int client) {
	// Cria um mutex para esperar que chegue a nova informacao
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	// Bloqueia o mutex temporario
	pthread_mutex_lock(&mutex);
	// Informa o programa que está à espera de informação numa determinada região
	pthread_mutex_lock(&waitingThreadsMutex);
	waitingThreads[messageReceived.region]++;
	pthread_mutex_unlock(&waitingThreadsMutex);
	// Bloqueia a thread à espera de nova informação
	pthread_cond_wait(&condClipboard[messageReceived.region], &mutex);

	pthread_mutex_unlock(&mutex);

	// Envia a informação para o cliente
	if(paste(messageReceived, client, APP) != 1) {
		return -1;
	} 

	return 1;
}

/**
 * Realiza pastes da informação contida no clipboard de modo a atualizar a inforamção do debaixo
 * @param  client file descriptor do cliente
 * @return        retorna 1 em caso de sucesso
 */
int backup(int client) {
	Message_struct message;

	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		// Região critica clipboard - leitura
		pthread_rwlock_rdlock(&rwlockClipboard);
		// Caso o clipboard tenha informação
		if(clipboard[i].size != 0) {
			pthread_rwlock_unlock(&rwlockClipboard);
			message.region = i;
			// Envia a informação para o clipboard filho
			if(paste(message, client, CLIPBOARD) == -1) {
				printf("Can't paste backup, region %d\n", i);
				exit(-2);
			}
		}
		else {
			pthread_rwlock_unlock(&rwlockClipboard);
		}
	}

	return 1;
}



/**
 * Thread responsavel por receber os pedidos dos clientes e fazer as ações por este pretendidas
 * @param  arg estrutura com as informacoes sobre a thread
 */
void * clientThread(void * arg) {
	thread_info_struct *threadInfo = arg;
	int client = threadInfo->inputArgument;

	// Caso a thread seja iniciada para responder a um clipboard, envia lhe o backup da informacao corrente
	if(threadInfo->type == CLIPBOARD) {
		backup(client);

		// Região critica lista de threads - escrita
		pthread_mutex_lock(&threadListMutex);
		// Adiciona o clipboard à lista de threads para propagar a informação
		clipboardThreadListAdd(threadInfo);
		pthread_mutex_unlock(&threadListMutex);
	}

	Message_struct message;

	// Loop de comunicacao com o cliente
	while(killSignal == 0) {		
		int numberOfBytesReceived = read(client, &message, sizeof(Message_struct));
		// Em caso de erro ou EOF, fecha a conecção
		if(numberOfBytesReceived <= 0) {
			break;
		}
		else if(message.action == COPY) {
			if(copy(message, client, threadInfo->type) <= 0) {
				break;
			}
		}
		else if(message.action == PASTE) {
			if(paste(message, client, threadInfo->type) <= 0) {
				break;
			}
		}
		else if(message.action == WAIT) {
			if(wait(message, client) <= 0) {
				break;
			}
		}
	}

	close(client);

	// Caso seja um cliente, libertar a memoria
	if(threadInfo->type == APP) {
		free(threadInfo);
	}
	
	printf("GoodBye - client\n");

	return NULL;
}

/**
 * Le a informação - mensagem mais data - de um clipboard de cima
 * @param  message messagem com os dados sobre a nova informacao
 * @param  data    vetor com a inforamcao
 * @return         -1 erro; 0 sucesso
 */
int readUp(int client, void * data, size_t size) {
	int receivedBytes = 0;

	pthread_rwlock_rdlock(&rwlockModeOfFunction);
	// Caso o clipboard tenha um pai, le a informacao por socket
	if(modeOfFunction == ONLINE) {
		pthread_rwlock_unlock(&rwlockModeOfFunction);
		receivedBytes = readAll(client, data, size);
		// Se cliente mandar EOF - termina a conecção
		if(receivedBytes <= 0) {
			// Critical Region - Write
			pthread_rwlock_wrlock(&rwlockModeOfFunction);
			modeOfFunction = LOCAL;
			pthread_rwlock_unlock(&rwlockModeOfFunction);
			printf("Changed to LOCAL MODE\n");
			return -1;
		}
		if(receivedBytes != size) {
			perror("readUp");
			printf("received Bytes %d %lu\n", receivedBytes, size);
			exit(-2);
		}
	}
	// Caso o clipboard esteja em modo local, 
	// le a informacao emviada pela client threads
	else if(modeOfFunction == LOCAL) {
		pthread_rwlock_unlock(&rwlockModeOfFunction);
		if(read(pipeThread[0], data, size) != size) {
			perror("readUp");
			exit(-2);
		}
	}
	else {
		pthread_rwlock_unlock(&rwlockModeOfFunction);
	}

	// Devolve o número de bytes recebido
	return receivedBytes;
}

/**
 * Escreve a informação para o clipboard acima deste, ou para o pipe quando em modo local
 * @param  message messagem com os dados sobre a nova informacao
 * @param  data    vetor com a inforamcao
 * @return         -1 erro; 0 sucesso
 */
int writeUp(Message_struct message, char * data) {
	pthread_rwlock_rdlock(&rwlockModeOfFunction);
	// Caso o clipboard tenha um pai, envia lhe a informacao por socket inet
	if(modeOfFunction == ONLINE) {
		pthread_rwlock_unlock(&rwlockModeOfFunction);
		if(write(sock_fd_inetIP, &message, sizeof(Message_struct)) != sizeof(Message_struct)) {
			perror("copy");
			return -1;
		}

		if(writeAll(sock_fd_inetIP, data, message.size) != message.size) {
			perror("copy");
			return -1;
		}
	}
	// Caso esteja em modo loca, escreve a informação para um pipe para ser lida pela upThread
	else if(modeOfFunction == LOCAL) {
		pthread_rwlock_unlock(&rwlockModeOfFunction);
		if(write(pipeThread[1], &message, sizeof(Message_struct)) != sizeof(Message_struct)) {
			perror("copy");
			return -1;
		}

		if(writeAll(pipeThread[1], data, message.size) != message.size) {
			perror("copy");
			return -1;
		}
	}
	else {
		pthread_rwlock_unlock(&rwlockModeOfFunction);
	}

	return 0;
}

/**
 * Thread responsavel por receber as atualizações vindas de cima da arvore, guardar no clipboard local
 *  e as propagar para baixo na arvore de clipboards
 * 			em modo ONLINE - le do socket 		em modo LOCAL - le do pipe
 * @param  arg estrutura com as informacoes sobre a thread
 * @return     [description]
 */
void * upThread(void * arg) {
	thread_info_struct *threadInfo = arg;
	int client = threadInfo->inputArgument;

	Message_struct message;

	int receivedBytes = 0;

	// Loop de comunicacao com o cliente
	while(killSignal == 0) {
		// Le a mensagem que o clipboard superior está a enviar
		receivedBytes = readUp(client, &message, sizeof(Message_struct));
		if(receivedBytes == -1) {
			continue;
		}

		// Ponteiro para a nova informação
		char *data = NULL;
		// Allocs memory to store new data
		data = (char *)malloc(message.size);
		if(data == NULL) {
			perror("malloc");
			exit(-1);
		}

		// Le a nova informação proveniente do clipboard superior
		receivedBytes = readUp(client, data, message.size);
		if(receivedBytes == -1) {
			continue;
		}

		// Replica a nova informação
		char *aux = (char *)malloc(message.size);
		memcpy(aux, data, message.size);

		// Regilao critica clipboard - escrita
		pthread_rwlock_wrlock(&rwlockClipboard);
		// Guarda a nova informação no clipboard
		if(clipboard[message.region].data != NULL) {
			free(clipboard[message.region].data);
		}
		clipboard[message.region].data = data;
		clipboard[message.region].size = message.size;
		//printf("Received %lu bytes - data: %s region: %d\n", message.size, clipboard[message.region].data, message.region);
		pthread_rwlock_unlock(&rwlockClipboard);

		pthread_mutex_lock(&waitingThreadsMutex);
		while(waitingThreads[message.region] > 0)
		{
			pthread_cond_signal(&condClipboard[message.region]);
			waitingThreads[message.region]--;
		}
		pthread_mutex_unlock(&waitingThreadsMutex);

		// Região crítica lista de threads - escrita e leitura
		pthread_mutex_lock(&threadListMutex);
		thread_info_struct *sendThreads = clipboardThreadList;
		thread_info_struct *auxList = NULL;

		while(sendThreads != NULL) {
			auxList = sendThreads->next;
			if(write(sendThreads->inputArgument, &message, sizeof(Message_struct)) != sizeof(Message_struct)) {
				printf("Client disconected\n");
				clipboardThreadListRemove(sendThreads->thread_id);
			}
			else if(writeAll(sendThreads->inputArgument, aux, message.size) != message.size) {
				printf("Client disconected\n");
				clipboardThreadListRemove(sendThreads->thread_id);
			}

			// Passes to the next clipboard
			sendThreads = auxList;
		}
		pthread_mutex_unlock(&threadListMutex);
		
		// Liberta o vetor auxiliar
		free(aux);
		
	}

	close(client);
	free(threadInfo);

	printf("GoodBye - upThread\n");

	return NULL;
}

/**
 * Thread responsavel por aceitar conecçoes debaixo e atualizar 
 * estas com a informação que existe no clipboard
 * @param  arg estrutura com as informacoes sobre a thread
 */
void * downThread(void * arg) {
	thread_info_struct *threadInfo = arg;
	int client = threadInfo->inputArgument;

	socklen_t size_addr;

	size_addr = sizeof(struct sockaddr);
	
	while(killSignal == 0) {
		// Aceita a conecção de clipboards
		int clipboard_client = accept(client, (struct sockaddr *) &clientClipboard_addr, &size_addr);
		if(clipboard_client != -1) {
			// Aloca uma estrutura com as informações de arranque de uma thread
			thread_info_struct *threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
			if(threadInfo == NULL) {
				perror("malloc");
				exit(0);
			}
			threadInfo->inputArgument = clipboard_client;
			threadInfo->type = CLIPBOARD;
			// Inicia uma nova thread para comunicar com o cliente
			pthread_create(&threadInfo->thread_id, NULL, &clientThread, threadInfo);
			printf("Accepted connection of clipboard\n");
			// Reinicia a variavel para poder corretamente fazer accept
			size_addr = sizeof(struct sockaddr);	
		}

	}
	// Limpa a estrutra dá própria thread
	free(threadInfo);
	printf("GoodBye - downThread\n");

	return NULL;
}