#ifndef LIBS_H
#define LIBS_H

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

#define PORT 65530
#define MAX_MESSAGE_SIZE 1024
#define TABLE_SIZE 100
#define DEFAULT_PATH "/"
#define MAX_THREADS 5
#define MAX_RETRIES 5
#define PPID_SIZE 5
#define B_PORT 9990
#define RECV_PORT 9995


#endif