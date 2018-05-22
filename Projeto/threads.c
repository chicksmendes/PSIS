#include "threads.h"

/**
 * Thread responsavel por receber os pedidos dos clientes e fazer as acoes pretendidas
 * @param  arg [description]
 * @return     [description]
 */
void * clientThread(void * arg) {
	thread_info_struct *threadInfo = arg;
	int client = threadInfo->inputArgument;

	/*Message_struct message;

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
		}
		else if(message.action == PASTE) {
			if(paste(message, client) == 0) {
				printf("Error on pasting\n");
			}
		}
		else if(message.action == WAIT) {
			if(wait(message, client) == 0) {
				printf("Error on wait\n");
			}
		}
	}*/
	close(client);
	printf("GoodBye - clientThread\n");

	return NULL;
}