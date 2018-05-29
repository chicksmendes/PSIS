#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#define MAX_INPUT 1000

int main(int argc, char const *argv[]) {
	char message[MAX_INPUT] = "";
	int region = 0;
	int action;
	int type = 0;
	int many;

	int clipboard_id = clipboard_connect(SOCKET_ADDR);
	if (clipboard_id == -1){
		printf("Error opening the FIFO files\n");
		exit(-1);
	}
	
	while(1) {
		printf("\nPress:\n\t1 to Copy\n\t2 to Paste\n\t3 to Wait\n\t4 to close\n");
		
		fgets(message,MAX_INPUT,stdin);
		sscanf(message, "%d", &action);
		if (action == 4) {
			clipboard_close(clipboard_id);
			break;
		}
		
		printf("FILE [1] or MESSAGE [2]");
		fgets(message,MAX_INPUT,stdin);
		sscanf(message, "%d", &type);
		// COPY
		if (action == 1) {
			// FILE
			if(type == 1) {
				printf("Name of the file: ");
				fgets(message,MAX_INPUT,stdin);
				
				FILE *file = NULL;

				file = fopen(message, "r");
			}
		}
	}



}
