#ifndef UDP_SERVER_H
#define UDP_SERVER_H
#include "libs.h"

char *to_addr (struct sockaddr_in *rec_addr);

char *get_path(char *key, char **table);

void my_exit();

int create_socket();

void bind_socket(int sock_fd, struct sockaddr_in sock_addr);

struct sockaddr_in create_addr(char *in_addr, int port);

#endif
