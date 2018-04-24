#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
	int fd;
    char *fifo = "escritaBonita";
    char str[100];

	mkfifo(fifo, 0666);
    // Open fifo file
    fd = open(fifo, O_WRONLY);
    if(fd == -1) {
    	 // Creates the fifo
    	perror("open");
    	exit(-1);
    }
    printf("FIFO created\n");

    while(1) {
    	fgets(str, 100, stdin);
    	write(fd, str, sizeof(str));
    	printf("%s\n", str);
    }

    // Close fifo
    close(fd);

    // Removes the link to the fifo
    unlink(fifo);

    return 0;
}
