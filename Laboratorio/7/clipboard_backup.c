#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"

int sock_fd_inet;

// Clipboard data
clipboard_struct clipboard;
// New data received
char *data = NULL;

// Unlinks the sockets when the program stops
void ctrl_c_callback_handler(int signum){
	printf("Caught signal Ctr-C\n");
	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		free(clipboard.clipboard[i]);
	}
	close(sock_fd_inet);
	exit(0);
}

int main(int argc, char const *argv[])
{
	int error = 0;
	int success = 1;
	struct sockaddr_in local_addr;
	struct sockaddr_in client_addr;

	socklen_t size_addr;

	// Atach the ctrl_c_callback_handler to the SIGINT signal
	signal(SIGINT, ctrl_c_callback_handler);

	if(argc != 2) {
		printf("Backup with incorrect initialization\n");
		exit(0);
	}

	sock_fd_inet = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd_inet == -1) {
		perror("socket");
		exit(-1);
	}


	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(atoi(argv[1]));
	local_addr.sin_addr.s_addr= INADDR_ANY;
	int err = bind(sock_fd_inet, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}
	printf("Socket created and binded\n");

	listen(sock_fd_inet, 5);

	printf("Ready to accept connections\n");

	Message_struct messageClipboard;

	// Init the clipboard struct
	for (int i = 0; i < 10; i++)
	{
		clipboard.size[i] = 0;
		clipboard.clipboard[i] = NULL;
	}

	while(1) {
		printf(".\n");

		// Reset hold variable
		size_addr = sizeof(struct sockaddr);

		int clipboard_client = accept(sock_fd_inet, (struct sockaddr *) & client_addr, &size_addr);

		printf("Clipboard connected\n");

		while(1) {
			int numberOfBytesReceived = read(clipboard_client, &messageClipboard, sizeof(Message_struct));
			if(numberOfBytesReceived == 0) {
				printf("Clipboard disconected\n");
				break;
			}

			if(numberOfBytesReceived != sizeof(Message_struct)) {
				printf("Didn't received the right message\n");
				continue;
			}

			if(messageClipboard.action == BACKUP) {
				printf("BACKUP\n");
				
				for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
				{
					messageClipboard.size[i] = clipboard.size[i];
				}

				// Sends the amount of data present in the clipboard
				write(clipboard_client, &messageClipboard, sizeof(Message_struct));

				for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
				{
					if(messageClipboard.size[i] != 0) {
						printf("region %d ", i);
						printf("clipboard content %s size %d\n", clipboard.clipboard[i], (int ) clipboard.size[i]);
						write(clipboard_client, clipboard.clipboard[i], clipboard.size[i]);
					} 
				}
			}
			if(messageClipboard.action == COPY) {
				//printf("Received information - action: COPY\n");
				printf("\nCOPY\n");
				// Allocs memory to store new data
				data = (char *)malloc(sizeof(char)*messageClipboard.size[messageClipboard.region]);
				if(data == NULL) {
					printf("Error alocating memory\n");
					write(clipboard_client, &error, sizeof(int));
					break;
				}

				// Informs the client that as allocated memory to receive the data
				write(clipboard_client, &success, sizeof(int));

				// Store the size of the clipboard region
				clipboard.size[messageClipboard.region] = sizeof(char)*messageClipboard.size[messageClipboard.region];

				// Receives the data from the client
				int numberOfBytesCopied = read(clipboard_client, data, clipboard.size[messageClipboard.region]);

				// Erases old data
				if(clipboard.clipboard[messageClipboard.region] != NULL) {
					printf("Region cleared\n");
					free(clipboard.clipboard[messageClipboard.region]);
				}

				// Assigns new data to the clipboard
				clipboard.clipboard[messageClipboard.region] = data;

				printf("Received %d bytes in region %d - data: %s\n", numberOfBytesCopied, messageClipboard.region, clipboard.clipboard[messageClipboard.region]);
			
			}
		}
		close(clipboard_client);
	}
}