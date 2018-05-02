#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <string.h>

#include <pthread.h>

int isPrime(int number) {
	int flag = 0;

	if(number == 0) {
		return 1;
	}
	if(number == 1) {
		return 1;
	}

	for(int i=2; i<=number/2; ++i)
    {
        // condition for nonprime number
        if(number % i == 0) {
            return 1;
        }
    }

    if (flag==0){
        return 0;
    }
}

void * countPrimes(void * fd) {
	int *pipefd = (int *) fd;
	int number;
	int total = 0;
	int flag;
	// Close the write 

	while(read(pipefd[0], &number, sizeof(int))) {
		if(isPrime(number) == 0) {
			total++;
		}
	}

	return total;
}

int main(int argc, char const *argv[])
{
	int numberOfThreads = atoi(argv[1]);
	int numberOfPrimes = 0;
	int retVal = 0;
	int number = atoi(argv[2]);
	int pipefd[2];
	pthread_t *thread_id = NULL;

	// Confirms if the number of arguments is correct
	if(argc != 3) {
		printf("Number of arguments if incorrect\n");
		exit(0);
	}

	// Creates the pipe
	if (pipe(pipefd) == -1) {
	   perror("pipe");
	   exit(EXIT_FAILURE);
	}



	thread_id = (pthread_t *)malloc(numberOfThreads*sizeof(pthread_t));
	if(thread_id == NULL) {
		perror("thread");
		exit(0);
	}

	// Creates the thread
	for (int i = 0; i < numberOfThreads; ++i)
	{
		pthread_create(&thread_id[i], NULL, countPrimes, (void *) pipefd);
		printf("New thread created with thread_id %d \n", (int) thread_id[i]);
	}

	// Closes the read from the pipe

	// Sends number to the pipe
	for (int i = 0; i < number; ++i)
	{
		write(pipefd[1], &i, sizeof(int));
	}

	close(pipefd[1]);
	// Waits for the threads to finish
	for (int i = 0; i < numberOfThreads; ++i)
	{
		pthread_join(thread_id[i], &retVal);
		numberOfPrimes += retVal;
		printf("Thread id %d count %d primes\n", (int)thread_id[i], retVal);
	}

	printf("Number of primes count %d\n", numberOfPrimes);
	close(pipefd[0]);
	return 0;
}