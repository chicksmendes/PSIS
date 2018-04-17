#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(){
	int action, region, copyData, pasteData;
	char aux[50], dadosSent[10], dadosReceived[10];

	// Connects to the cliboard
	int sock_fd = clipboard_connect("./");
	if(sock_fd == -1){
		exit(-1);
	}

	while(1) {
		// Ask the user the action that wants to perform
		printf("\nCOPY [0] or PASTE [1]: ");
		fgets(aux, 50, stdin);
		sscanf(aux, "%d", &action);

		// Copies the data read to the clipboard server
		if(action == COPY) {
			// Ask the user for some data to be stored
			printf("Write something: ");
			fgets(dadosSent, 10, stdin);
			// Terminates the received string
			dadosSent[strlen(dadosSent) - 1] = '\0';

			// Ask the region to store the data
			printf("Region [0-9]: ");
			fgets(aux, 50, stdin);
			sscanf(aux, "%d", &region);

			// Sends the data to the cliboard server
			copyData = clipboard_copy(sock_fd, region, dadosSent, strlen(dadosSent) + 1);
			if(copyData < 1) {
				printf("Error on copy\n");
			}
			else {
				printf("Sent %d - data: %s\n", copyData, dadosSent);
			}
		}
		else if(action == PASTE) {
			// Ask the region from where will paste data
			printf("Region [0-9]: ");
			fgets(aux, 50, stdin);
			sscanf(aux, "%d", &region);

			// Sends the information to the clipboard
			pasteData = clipboard_paste(sock_fd, region, dadosReceived, 10*sizeof(char));
			if(pasteData < 1) {
				printf("Didn't receive paste information\n");
			}
			else {
				printf("Received %d - data: %s\n", pasteData, dadosReceived);
			}
		}
		else {
			printf("Closing connection\n");
			break;
		}
	}

	/*char dados[10];
	int dados_int;
	fgets(dados, 10, stdin);
	write(fd, dados, 10);
	read(fd+1, &dados_int, sizeof(dados_int));
	printf("Received %d\n", dados_int);*/
	
	close(sock_fd);
	exit(0);
}
