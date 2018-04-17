#define INBOUND_FIFO "INBOUND_FIFO"
#define OUTBOUND_FIFO "OUTBOUND_FIFO"
#define PASTE 1
#define COPY 0
#define NUMBEROFPOSITIONS 10
#define SOCKET_ADDR "./CLIPBOARD_SOCKET"

#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct _message {
	size_t size;
	int region;
	int action;
} Message_struct;

int clipboard_connect(char * clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
