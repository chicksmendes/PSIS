#include "threads.h"

extern int killSignal;

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
        if (sentBytes <= 0) { 
        	return -1; 
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
        if (receiveBytes <= 0) { 
        	return -1; 
        }
        total += receiveBytes;
        bytesleft -= receiveBytes;
    }

    // Returns -1 if cannot receive information, returns total bytes receive otherwise
    return receiveBytes == -1?-1:total; 
}



/**
 * Thread responsavel por receber os pedidos dos clientes e fazer as acoes pretendidas
 * @param  arg [description]
 * @return     [description]
 */
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
		}/*
		else if(message.action == PASTE) {
			if(paste(message, client) == 0) {
				printf("Error on pasting\n");
			}
		}
		else if(message.action == WAIT) {
			if(wait(message, client) == 0) {
				printf("Error on wait\n");
			}
		}*/
	}
	close(client);
	printf("GoodBye - clientThread\n");

	return NULL;
}