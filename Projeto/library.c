#include "clipboard.h"
#include "clipboardIntern.h"

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
int readEverything(int sock_fd, char *buf, int len) {
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
 * Connects to a fifo with the name declared as input
 * @param  clipboard_dir descrição do path do file descriptor do fifo
 * @return               valor do fifo para aceder ao clipboard, -1 em caso de erro
 */
int clipboard_connect(char * clipboard_dir){
	// Socket Structs
	struct sockaddr_un server_addr;
	struct sockaddr_un client_addr;

	// Create Socket
	int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock_fd == -1) {
		perror("socket");
		return(-1);
	}

	printf("Socket opened\n");
	
	// Atribution of a name to the socket
	// Client
	client_addr.sun_family = AF_UNIX;
	sprintf(client_addr.sun_path, "%ssocket_%d", clipboard_dir, getpid());
	// Server
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, SOCKET_ADDR);

	int err_c = connect(sock_fd, (const struct sockaddr *) &server_addr, sizeof(struct sockaddr_un));
	if(err_c == -1) {
		printf("Error connecting\n");
		return(-1);
	}

	printf("Conected to server\n");

	return sock_fd;
}

/** 
 * Copia a informação apontada por buf, com tamanho count para uma regiao do clipboard
 * @param  clipboard_id número do fifo devlvido pelo connect
 * @param  region       regiao onde se quer guardar a informacao
 * @param  buf          ponteiro para a data
 * @param  count        número de bytes da data
 * @return              número de bytes copiados em caso de sucesso, -1 em caso de erro
 */
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count) {
	Message_struct message;
	int numberOfBytesSent = 0;
	int status = 1;

	// Se o ponteiro tiver vazio, não faz copy
	if(buf == NULL) {
		printf("Buffer is pointing to NULL\n");
		return 0;
	}

	if(count < 0) {
		printf("Count infirior to zero\n");
		return 0;
	}

	// Confirma a região
	if(region > NUMBEROFPOSITIONS-1 || region < 0) {
		printf("Invalid Region %d\n", region);
		return 0;
	}

	// Carrega a mensagem com a acao pretendida
	message.region = region;
	message.size = count;
	message.action = COPY;

	// Informa o clipboard da ação que pretende realizar com ele - COPY
	if(write(clipboard_id, &message, sizeof(Message_struct)) != sizeof(Message_struct)) {
		printf("Cannot write to clipboard\n");
		return 0;
	}
	// Le se o clipboard conseguiu alocar memoria para a mesagem que se quer enviar
	if(read(clipboard_id, &status, sizeof(int)) != sizeof(int)) {
		printf("Cannot read from clipboard\n");
		return 0;
	}
	// Clipboard conseguiu alocar a memoria e está pronto a receber a informação
	if(status == 1) {
		// Envia a informação para o clipboard
		numberOfBytesSent =  write(clipboard_id, buf, count*sizeof(char));

		// Verifica se a informação foi bem enviada
		if(numberOfBytesSent != count*sizeof(char)) {
			printf("Didn't send all the information to the clipboard\n");
			return 0;
		}
	}
	else {
		// Server cannot received data
		printf("Server cannot received data\n");
		return 0;
	}

	return numberOfBytesSent;
}

/**
 * Devolve a informação de uma região do clipboard, guarda na variavel apontada por buf, com tamanho count
 * @param  clipboard_id número do fifo devlvido pelo connect
 * @param  region       regiao onde se quer obter a informacao
 * @param  buf          ponteiro para a data
 * @param  count        número de bytes da data
 * @return              número de bytes obtidos do clipboard em caso de sucesso, -1 em caso de erro
 */
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count) {
	Message_struct message;
	int numberOfBytesReceived = 0;
	int status = 0;

	// Se o ponteiro tiver vazio, não faz paste
	if(buf == NULL) {
		printf("Buffer is pointing to NULL\n");
		return 0;
	}

	// Confirma a região
	if(region > NUMBEROFPOSITIONS - 1 || region < 0) {
		printf("Invalid Region\n");
		return 0;
	}

	// Carrega a mensagem com a ação pretendida
	message.region = region;
	message.size = (size_t) count;
	message.action = PASTE;

	// Informs the server of the action that the client wants to take - PASTE
	if(write(clipboard_id, &message, sizeof(Message_struct)) != sizeof(Message_struct)) {
		printf("Cannot write to clipboard\n");
		return 0;
	}

	// Reads if the slient has enough space to store the PASTE data
	if(read(clipboard_id, &status, sizeof(int)) != sizeof(int)) {
		printf("Cannot read from clipboard\n");
		return 0;
	}
	if(status == 1) {
		// Reads the information about the message stored at the cliboard
		if(read(clipboard_id, &message, sizeof(Message_struct)) != sizeof(Message_struct)) {
			printf("Cannot write to clipboard\n");
			return 0;
		}

		// Reads the data from the clipboard
		numberOfBytesReceived = readEverything(clipboard_id, buf, message.size*sizeof(char));
	}
	else {
		printf("Don't have enough space to store data\n");
		return 0;
	}
	return numberOfBytesReceived;
}

/**
 * Espera por uma nova informação numa regiao do clipboard. 
 * Devolve a informação de uma região do clipboard, guarda na variavel apontada por buf, com tamanho count
 * @param  clipboard_id número do fifo devlvido pelo connect
 * @param  region       regiao onde se quer obter a informacao
 * @param  buf          ponteiro para a data
 * @param  count        número de bytes da data
 * @return              número de bytes obtidos do clipboard em caso de sucesso, -1 em caso de erro
 */
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count) {
	Message_struct message;
	int status = 0;
	int receivedBytes = 0;

	if(buf == NULL) {
		printf("Buffer is pointing to NULL\n");
		return 0;
	}

	if(region > NUMBEROFPOSITIONS - 1 || region < 0) {
		printf("Wait region out of bounds\n");
		return 0;
	}

	// Loads the stuct with the information
	message.region = region;
	message.size = count;
	message.action = WAIT;

	// Informs the server of the action that the client wants to take - PASTE
	if(write(clipboard_id, &message, sizeof(Message_struct)) != sizeof(Message_struct)) {
		return 0;
	}

	printf("Waiting for the new information\n");

	// Reads if the slient has enough space to store the PASTE data
	if(read(clipboard_id, &status, sizeof(int)) != sizeof(int)) {
		printf("Cannot read from clipboard\n");
		return 0;
	}
	if(status == 1) {
		// Reads the information about the message stored at the cliboard
		if(read(clipboard_id, &message, sizeof(Message_struct)) != sizeof(Message_struct)) {
			printf("Cannot write to clipboard\n");
			return 0;
		}

		// Reads the data from the clipboard
		receivedBytes = readEverything(clipboard_id, buf, message.size*sizeof(char));
	}
	else {
		printf("Don't have enough space to store data\n");
		return 0;
	}

	return(receivedBytes);
}

/**
 * Fecha a conecção com o clipboard
 * @param clipboard_id desciptor do connect
 */
void clipboard_close(int clipboard_id) {
	close(clipboard_id);
}