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
	int i = 0;
	int fd;
	int n;
    char *fifo = "escritaBonita";
    char buf[100];

    // Open the fifo file
    fd = open(fifo, O_RDONLY);
    while(read(fd, buf, 100)) {	
		for(i = 0; i < strlen(buf); i++) {
			buf[i] = toupper(buf[i]);
		}
    	printf("Received: %s\n", buf);

    } 
    
    close(fd);

    return 0;
}
