#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char const *argv[])
{
	int pipefd[2];
	pid_t cpid;
	int buf;
	int numberOfChilds = 0;
	int number = 0;
	int numberOfPrimes = 0;
	int status;
	int flag = 0;

	if (argc != 3) {
	   fprintf(stderr, "Usage: %s <string>\n", argv[0]);
	   exit(EXIT_FAILURE);
	}

	// Creates the pipe
	if (pipe(pipefd) == -1) {
	   perror("pipe");
	   exit(EXIT_FAILURE);
	}

	// Reads the number os childs
	if(sscanf(argv[1], "%d", &numberOfChilds) < 0) {
		printf("ERROR READING NB OF CHILDS\n");
		exit(0);
	}

	// Assing the number to analise
	if(sscanf(argv[2], "%d", &number) < 0) {
		printf("ERROR READING NB TO ANALISE\n");
		exit(0);
	}

	// Creates child processes
	for (int i = 0; i < numberOfChilds; ++i)
	{
		cpid = fork();
		if (cpid == -1) {
		   perror("fork");
		   exit(EXIT_FAILURE);
		}
		if(cpid == 0) {
			break;
		}
	}

	// Code run by the child
	if (cpid == 0) {
		// Fecha a escrita
	    close(pipefd[1]);         

		numberOfPrimes=0;

		// Le do buffer
	    while (read(pipefd[0], &buf, sizeof(int)) > 0) {
	   		flag = 0;
	   		// Analisa se o numero é primo
			for(int i=2; i<=buf/2; ++i)
			{
				// condition for nonprime number
				if(buf%i==0)
				{
				    flag = 1;
				    break;
				}
			}
			if(flag == 0) {
				numberOfPrimes++;
			}
	   }
	   // Fecha a leitura
	   close(pipefd[0]);
	   printf("SLAVE id=%d, number of primes=%d\n", getpid(), numberOfPrimes);
	   _exit(EXIT_SUCCESS);
	}
	// Code runned by the parent
	else {            
		// Fecha a leitura 
		close(pipefd[0]);

		for (int i = 2; i < number; ++i)
		{
			write(pipefd[1], &i, sizeof(int));
		}
		// Fecha a escrita
		close(pipefd[1]);
		// Espera pelos filhos terminarem
		while (wait(&status) > 0);
		exit(EXIT_SUCCESS);
	}

	return 0;
}