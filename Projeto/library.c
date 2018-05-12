#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "clipboard.h"


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




/**
 * @brief      { function_description }
 *
 * @param      clipboard_dir  The clipboard dir
 *
 * @return     { description_of_the_return_value }
 */
int clipboard_connect(char * clipboard_dir){
	char socketName[100];

	// Socket Structs
	struct sockaddr_un server_addr;
	struct sockaddr_un client_addr;

	// Create Socket
	int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock_fd == -1) {
		perror("socket");
		exit(-1);
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
		exit(-1);
	}

	printf("Conected to server\n");

	return sock_fd;
}

/**
 * @brief      Copies the buffer to a determinated region on the clipboard
 *
 * @param[in]  clipboard_id  The file descriptor to the socket
 * @param[in]  region        The region on the buffer will be copied
 * @param      buf           The buffer
 * @param[in]  count         The size of the information
 *
 * @return     In case of success returns the number of bytes copied or -1 in case of fail
 */
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count){
	Message_struct message;
	int numberOfBytesSent = 0;
	int status = 1;

	if(region > NUMBEROFPOSITIONS) {
		return 0;
	}

	// Loads the stuct with the information
	message.region = (size_t) region;
	message.size[message.region] = count;
	message.action = COPY;

	// Informs the server of the action that the client wants to take - COPY
	write(clipboard_id, &message, sizeof(Message_struct));

	if(status == 1) {
		// Sends the data to the clipboard
		numberOfBytesSent =  writeAll(clipboard_id, buf, count*sizeof(char));

		// Tests if all the information was sent
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
		
	//printf("Sent %d bytes - data: %s\n", numberOfBytesSent, (char *) buf);

	return numberOfBytesSent;
}
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){
	Message_struct message;
	int numberOfBytesReceived = 0;
	int status = 0;

	if(region > NUMBEROFPOSITIONS) {
		printf("Paste region out of bounds\n");
		return 0;
	}

	// Loads the stuct with the information
	message.region = region;
	message.size[message.region] = (size_t) count;
	message.action = PASTE;

	// Informs the server of the action that the client wants to take - PASTE
	write(clipboard_id, &message, sizeof(Message_struct));

	// Reads if the slient has enough space to store the PASTE data
	read(clipboard_id, &status, sizeof(int));
	if(status == 1) {
		// Reads the information about the message stored at the cliboard
		read(clipboard_id, &message, sizeof(Message_struct));

		// Reads the data from the clipboard
		numberOfBytesReceived = readAll(clipboard_id, buf, message.size[message.region]*sizeof(char));

		//printf("Received %d bytes - data: %s\n", numberOfBytesReceived, (char *) buf);
	}
	else {
		return 0;
	}
	return numberOfBytesReceived;
}
