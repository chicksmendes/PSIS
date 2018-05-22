#define UPDATE_FIFO "UPDATE_FIFO"
#define BACKUP 3
#define WAIT 2
#define PASTE 1
#define COPY 0
#define NUMBEROFPOSITIONS 10
#define SOCKET_ADDR "./CLIPBOARD_SOCKET"
#define ONLINE_FLAG "-c"
#define UP 0
#define DOWN 1

#define LOCAL 0
#define ONLINE 1

#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>


typedef struct _clipboard {
	char *clipboard[NUMBEROFPOSITIONS];
	size_t size[NUMBEROFPOSITIONS];
} clipboard_struct;

typedef struct _message {
	size_t size[NUMBEROFPOSITIONS];
	int region;
	int action;
} Message_struct;

typedef struct _messageQueueStruct {
	struct _messageQueueStruct *next;
	int source;
	int region;
} messageQueueStruct;

typedef struct _thread_info_struct {
	pthread_t thread_id; 
	int inputArgument;
	struct _thread_info_struct *next;
} thread_info_struct;

typedef struct _updateMessage {
	int region;
	int source;
} updateMessage;

int clipboard_connect(char * clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);