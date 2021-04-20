#define _XOPEN_SOURCE
#define _GNU_SOURCE

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"

#define PORT 65532
#define MAX_MESSAGE_SIZE 1024
#define TABLE_SIZE 100
#define DEFAULT_PATH "/"
#define MAX_THREADS 5
#define PPID_SIZE 5
#define B_PORT 9994
#define RECV_PORT 9995

char *to_addr (struct sockaddr_in *rec_addr) {
    char *addr = (char *)calloc(25, 1);
    char *inet_addr = calloc(20, 1);
    sprintf(addr, "%s", inet_ntop(AF_INET, (const void *)&(rec_addr->sin_addr.s_addr), inet_addr, 20)); //, (unsigned short)ntohs(rec_addr->sin_port));
    free(inet_addr);
    return addr;
}
