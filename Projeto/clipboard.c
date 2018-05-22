#include "clipboardIntern.h"
#include "utils.h"

// Regioes do clipboard
clipboard_struct clipboard[NUMBEROFPOSITIONS];

// Unlinks the sockets when the program stops
void ctrl_c_callback_handler(int signum){
	printf("aught signal Ctr-C\n");
	// Closes the sockets
	/*close(sock_fd_inet);

	close(sock_fd_inetIP);

	close(sock_fd_unix);
	// É uma região critia - A TRATAR!!!!
	killSignal = 1;
	// Clears the clipboard
	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		free(clipboard.clipboard[i]);
	}
	
	unlink(SOCKET_ADDR);*/
	exit(0);
}

int main(int argc, char const *argv[])
{
	// Atach the ctrl_c_callback_handler to the SIGINT signal
	signal(SIGINT, ctrl_c_callback_handler);

	int portUp;
	char ip[14];

	if(argc == 1) {
		printf("Only local host\n");
		modeOfFunction = LOCAL;
	}
	else if(argc == 4) {
		// Server will function only as backup
		if(strcmp(argv[1], ONLINE_FLAG) != 0) {
			printf("Incorrect initialization\n");
			exit(0);
		}
		else {
			modeOfFunction = 1;
		}

		// Copy IP from the argv
		strcpy(ip, argv[2]);

		// Copy port from argv
		portUp = atoi(argv[3]);
	}
	else {
		printf("Incorrect initialization - number of arguments\n");
		exit(0);
	}

	int portDown = randomPort();

	// Imprime o porto para aceder ao clipboard
	printf("Port to acess machine: %d\n", portDown);

	// Inicializa o clipboard
	for (int i = 0; i < 10; i++)
	{
		clipboard[i].data = NULL;
		clipboard[i].size = 0;
	}

	return 0;
}