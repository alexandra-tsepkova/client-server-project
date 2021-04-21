#include "udp_lib.h"

char *to_addr (struct sockaddr_in *rec_addr) {
    char *addr = (char *)calloc(25, 1);
    char *inet_addr = calloc(20, 1);
    sprintf(addr, "%s", inet_ntop(AF_INET, (const void *)&(rec_addr->sin_addr.s_addr), inet_addr, 20)); //, (unsigned short)ntohs(rec_addr->sin_port));
    free(inet_addr);
    return addr;
}


char *get_path(char *key, char **table) { //get path with certain key from table
    for (int i = 0; i < TABLE_SIZE; ++i) {
        if (table[i] == NULL) {
            table[i] = calloc(1, sizeof(key) + 1);
            strcpy(table[i], key);
            table[TABLE_SIZE + i] = calloc(1, PATH_MAX);
            strcpy(table[TABLE_SIZE + i], DEFAULT_PATH);
            return table[TABLE_SIZE + i];
        } else if(strcmp(key, table[i]) == 0) {
            return table[TABLE_SIZE + i];
        }
    }
    exit(1);
}


void my_exit(){
    pr_err("Exiting with error\n");
    remove("/var/run/client_server.pid");
    exit(-1);
}

int create_socket() {
    int num_tries = 0;
    for (int i = 0; i < MAX_RETRIES; i++){
        int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_fd < 0) {
            pr_err("Can't create socket: try #%d\n", num_tries);
            sleep(1);
            num_tries++;
        }
        if(sock_fd > 0) {
            pr_info("Created socket at try #%d\n", num_tries);
            return sock_fd;
        }
    }
    my_exit();
}

void bind_socket(int sock_fd, struct sockaddr_in sock_addr){
    int num_tries = 0;
    for(int i = 0; i < MAX_RETRIES; i++) {
        if (bind(sock_fd, (const struct sockaddr *) (&sock_addr), (socklen_t) sizeof(sock_addr)) < 0) {
            pr_err("Can't assign address to a socket: try #%d\n", num_tries);
            sleep(1);
            num_tries++;
        }
        else {
            pr_info("Assigned address to a socket at try #%d\n", num_tries);
            return;
        }
    }
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