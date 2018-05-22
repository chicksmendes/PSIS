#include "utils.h"

/**
 * Gera o número do porto de forma aleatoria a partir do PID do processo
 * @return número do porto
 */
int randomPort() {
	// Creates port to comunnicate with clipboards down on the tree
	srand(getpid());   // seeds the port number on the pid of the process
	int portDown = rand()%(100) + 8000; 

	return portDown;
}