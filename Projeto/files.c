#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"

int main(){
	int action, copyData, pasteData, type;
	int filesize;
	char aux[200], dadosSent[100], dadosReceived[100];
	char filename[100];

	// Connects to the cliboard
	int sock_fd = clipboard_connect("./");
	if(sock_fd == -1){
		exit(-1);
	}

	while(1) {
		int region = -1;
		// Ask the user the action that wants to perform
		printf("\nPress: 0 to Copy | 1 to Paste | 2 to Wait | 3 to close\nAction: ");
		fgets(aux, 50, stdin);
		sscanf(aux, "%d", &action);

		if(action == 3) {
			clipboard_close(sock_fd);
			exit(0);
		}

		if(action > 3 || action < 0) {
			printf("Action Invalid\n");
			continue;
		}

		// Ask the region of the clipboard
		printf("Region [0-9]: ");
		fgets(aux, 100, stdin);
		sscanf(aux, "%d", &region);

		// Ask the type of file of the clipboard
		printf("String [0] or File [1]: ");
		fgets(aux, 200, stdin);
		sscanf(aux, "%d", &type);

		if(type == 1) {
			// Ask the name of the file
			printf("Name of the file with extension: ");
			fgets(filename, 100, stdin);

			// Terminates the received string
			filename[strlen(filename) - 1] = '\0';
		}

		// Copies the data read to the clipboard server
		if(action == COPY) {
			// COPY STRING
			if(type == 0) {
				// Ask the user for some data to be stored
				printf("Write something: ");
				fgets(dadosSent, 100, stdin);
				// Terminates the received string
				dadosSent[strlen(dadosSent) - 1] = '\0';

				// Sends the data to the cliboard server
				copyData = clipboard_copy(sock_fd, region, dadosSent, strlen(dadosSent) + 1);
				if(copyData < 1) {
					printf("Error on copy\n");
				}
				else {
					printf("Sent %d - data: %s\n", copyData, dadosSent);
				}
			}
			//COPY FILE
			if(type == 1) {
				FILE *file = NULL;

				file = fopen(filename, "r");
				
				int filesize;
				fseek(file, 0, SEEK_END);
				filesize = ftell(file);
				rewind(file);

				char *sendbuf = (char *) malloc (sizeof(char)*filesize);
				
				size_t result = fread(sendbuf, 1, filesize, file);

				if (result <= 0) {
					printf("Error fread\n");
					free(sendbuf);
					fclose(file);
					continue;
				}

				// Sends the data to the cliboard server
				copyData = clipboard_copy(sock_fd, region, sendbuf, sizeof(char)*filesize);
				if(copyData < 1) {
					printf("Error on copy\n");
				}
				else {
					printf("Sent %s - size: %d\n", filename, copyData);
				}

				fclose(file);
				free(sendbuf);
			}
		}
		else if(action == PASTE) {
			// PASTE STRING
			if(type == 0) {
				// Receives the information to the clipboard
				pasteData = clipboard_paste(sock_fd, region, dadosReceived, 100*sizeof(char));
				if(pasteData < 1) {
					printf("Didn't receive paste information\n");
				}
				else {
					printf("Received %d - data: %s\n", pasteData, dadosReceived);
				}
			}
			// PASTE FILE
			if(type == 1) {
				FILE *file = NULL;

				file = fopen(filename, "w");

				// Ask the syze of the file
				printf("Size of the file [bytes]: ");
				fgets(aux, 200, stdin);
				sscanf(aux, "%d", &filesize);

				char *receiveFile = (char *)malloc(sizeof(char)*filesize);

				// Receives the information to the clipboard
				pasteData = clipboard_paste(sock_fd, region, receiveFile, filesize*sizeof(char));
				if(pasteData < 1) {
					printf("Didn't receive paste information\n");
					fclose(file);
					free(receiveFile);
					continue;
				}

				printf("Received %s - size: %d\n", filename, pasteData);
				
				fwrite(receiveFile, 1, filesize, file);
				fclose(file);
				
				free(receiveFile);
			}
		}
		else if(action == WAIT) {
			if(type == 0) {
				// Sends the information to the clipboard
				pasteData = clipboard_wait(sock_fd, region, dadosReceived, 100*sizeof(char));
				if(pasteData < 1) {
					printf("Didn't receive paste information\n");
				}
				else {
					printf("Received %d - data: %s\n", pasteData, dadosReceived);
				}
			}
		}
	}
	
	close(sock_fd);
	exit(0);
}
