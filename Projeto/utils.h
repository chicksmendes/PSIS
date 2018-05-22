#ifndef UTILS_H
#define UTILS_H

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int randomPort();

int writeAll(int sock_fd, char *buf, int len);

int readAll(int sock_fd, char *buf, int len);

#endif