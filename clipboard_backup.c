#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"

int sock_fd_inet;

// Unlinks the sockets when the program stops
void ctrl_c_callback_handler(int signum){
	printf("Caught signal Ctr-C\n");
	close(sock_fd_inet);
	exit(0);
}

int main(int argc, char const *argv[])
{
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
	printf(" socket created and binded \n");

	listen(sock_fd_inet, 5);

	printf("Ready to accept connections\n");

	Message_struct_clipboard messageClipboard;

	// Clipboard data
	char *clipboard[10];
	int clipboardDataSize[10];
	// Init the clipboard struct
	for (int i = 0; i < 10; i++)
	{
		clipboardDataSize[i] = 0;
		clipboard[i] = NULL;
	}
	// New data received
	char *data = NULL;

	while(1) {
		printf(".\n");

		// Reset hold variable
		size_addr = sizeof(struct sockaddr);

		int clipboard_client = accept(sock_fd_inet, (struct sockaddr *) & client_addr, &size_addr);

		while(1) {
			int numberOfBytesReceived = read(clipboard_client, &messageClipboard, sizeof(Message_struct_clipboard));
			if(numberOfBytesReceived == 0) {
				printf("Clipboard disconected\n");
				break;
			}

			if(messageClipboard.action == BACKUP) {
				printf("BACKUP\n");
				

				// Sends the amount of data present in the clipboard
				write(clipboard_client, messageClipboard, sizeof(Message_struct_clipboard));

				for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
				{
					if(messageClipboard.size[i] != 0) {
						write(clipboard_client, clipboard[i], messageClipboard.size[i])
					} 
				}

			}
		}
	}
}