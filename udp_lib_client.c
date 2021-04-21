#include "udp_lib.h"

char *to_addr (struct sockaddr_in *rec_addr) {
    char *addr = (char *)calloc(25, 1);
    char *inet_addr = calloc(20, 1);
    sprintf(addr, "%s", inet_ntop(AF_INET, (const void *)&(rec_addr->sin_addr.s_addr), inet_addr, 20)); //, (unsigned short)ntohs(rec_addr->sin_port));
    free(inet_addr);
    return addr;
}


void my_exit(){
    printf("Exiting with error\n");
    exit(-1);
}

int create_socket() {
    int num_tries = 0;
    for (int i = 0; i < MAX_RETRIES; i++){
        int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_fd < 0) {
            sleep(1);
            num_tries++;
        }
        if(sock_fd > 0) {
            return sock_fd;
        }
    }
    printf("Can't create socket\n");
    my_exit();
}

void bind_socket(int sock_fd, struct sockaddr_in sock_addr){
    int num_tries = 0;
    for(int i = 0; i < MAX_RETRIES; i++) {
        if (bind(sock_fd, (const struct sockaddr *) (&sock_addr), (socklen_t) sizeof(sock_addr)) < 0) {
            sleep(1);
            num_tries++;
        }
        else {
            return;
        }
    }
    printf("Can't assign address to a socket");
    my_exit();
}

struct sockaddr_in create_addr(char *in_addr, int port){
    struct in_addr addr;
    addr.s_addr = inet_addr(in_addr);
    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(port);
    sock_addr.sin_addr = addr;
    return sock_addr;
}

