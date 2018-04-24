#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(){
		int action, region, copyData, receivedData;
		char dadosSent[10], dadosReceived[10];
		char aux[50];

		int fd = clipboard_connect("./");
		
		if(fd== -1){
			exit(-1);
		}

		while(1) {
			// Ask the action to be taken
			printf("\nDo you want to COPY (0) or PASTE (1)? ");-
			fgets(aux, 50, stdin);
			sscanf(aux, "%d", &action);
			// COPY
			if(action == COPY) {
				// Ask the data to store to the user
				printf("Write something: ");
				fgets(dadosSent, 10, stdin);
				dadosSent[strlen(dadosSent)-1] = '\0';

				// Ask the region
				printf("Region [0-9]: ");
				fgets(aux, 50, stdin);
				sscanf(aux, "%d", &region);

				// Sends the data to the clipboard
				copyData = clipboard_copy(fd, region, dadosSent, sizeof(dadosSent));
				if(copyData < 1) {
					printf("ERROR ON COPY\n");
				}
			}
			// PASTE
			else if(action == PASTE) {
				// Ask the region
				printf("Region [0-9]: ");
				fgets(aux, 50, stdin);
				sscanf(aux, "%d", &region);

				// Receives the data from the keyboard
				receivedData = clipboard_paste(fd+1, region, dadosReceived, strlen(dadosReceived)+1);	
				if(receivedData < 1) {
					printf("ERROR ON PASTE\n");
				}	
				printf("Data Received %s\n", dadosReceived);
			} 
			else {
				break;
			}
		}
		exit(0);
	}
