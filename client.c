#include "libs.h"

int main(int argc, char **argv) {
    char *cur_dir = calloc(PATH_MAX, sizeof(char));
    strcat(getcwd(cur_dir, PATH_MAX), "/log_client.txt");
    log_init(cur_dir);

    int ppid = getppid();

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        pr_err("Can't create socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    int server_len = sizeof(struct sockaddr_in);

    memset((void *) &server_addr, 0, server_len);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(argc < 2) {
        pr_err("Not enough arguments\n");
        return -1;
    }

    if(strcmp("find", argv[1]) == 0) {
        int b_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (b_sock_fd < 0) {
            pr_err("Can't create socket\n");
            return -1;
        }

        int yes = 1;
        int res = setsockopt(b_sock_fd, SOL_SOCKET, SO_BROADCAST, (char *) &yes, sizeof(yes));
        if (res == -1) {
            pr_err("Setsockopt error\n");
            exit(-1);
        }

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        res = setsockopt(b_sock_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout));
        if (res == -1) {
            pr_err("Setsockopt error\n");
            exit(-1);
        }

        struct sockaddr_in b_sock_addr;
        int b_addr_len = sizeof(struct sockaddr_in);

        memset((void *) &b_sock_addr, 0, b_addr_len);
        b_sock_addr.sin_family = AF_INET;
        b_sock_addr.sin_port = htons(B_PORT);
        b_sock_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

        char *b_buf = calloc(MAX_MESSAGE_SIZE, 1);
        strcpy(b_buf, "Is anybody here?");


        int b_sent = sendto(b_sock_fd, "Is anybody here?", strlen("Is anybody here?"), 0,
                            (const struct sockaddr *) &b_sock_addr, b_addr_len);
        if (b_sent < 0) {
            pr_err("Error while sending (broadcast)\n");
            return -1;
        }

        while (1) {
            struct sockaddr_in b_rec_addr;
            char *rec_buf = calloc(MAX_MESSAGE_SIZE, 1);
            int b_addr_size = sizeof(b_rec_addr);
            int b;
            b = recvfrom(b_sock_fd, rec_buf, MAX_MESSAGE_SIZE, 0, (struct sockaddr *restrict) &b_rec_addr,
                         (socklen_t *restrict) &b_addr_size);
            if (b < 0) {
                pr_err("Can't read message\n");
                exit(0);
            }
            printf("Server found: %s\n", to_addr(&b_rec_addr));
        }
    } else {
        char *buf = calloc(MAX_MESSAGE_SIZE, 1);
        sprintf(buf, "%05d", ppid);
        strcat(buf, argv[1]);
        if(argv[2]) {
            strcat(buf, " ");
            strcat(buf, argv[2]);
        }

        if(strncmp(buf + PPID_SIZE, "exit", 4) == 0) {
            server_addr.sin_port = htons(B_PORT);
        }

        int sent = sendto(sock_fd, buf, strlen(buf), 0, (struct sockaddr*)&server_addr, server_len);
        if(sent < 0) {
            pr_err("Error while sending\n");
            return -1;
        }

        int b_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);


        struct in_addr addr;
        addr.s_addr = inet_addr("0.0.0.0");
        struct sockaddr_in sock_addr;
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = htons(RECV_PORT);
        sock_addr.sin_addr = addr;

        if (bind(b_sock_fd, (const struct sockaddr*)(&sock_addr), (socklen_t)sizeof(sock_addr)) < 0) {
            pr_err("Can't assign address to a socket\n");
            close(sock_fd);
            exit(-1);
        }

        struct sockaddr_in b_rec_addr;
        char *rec_buf = calloc(MAX_MESSAGE_SIZE, 1);
        int b_addr_size = sizeof(b_rec_addr);
        int b;
        b = recvfrom(b_sock_fd, rec_buf, MAX_MESSAGE_SIZE, 0, (struct sockaddr *restrict) &b_rec_addr,
                     (socklen_t *restrict) &b_addr_size);
        if (b < 0) {
            pr_err("Can't read message\n");
            exit(0);
        }
        printf("%s\n", rec_buf);
    }
    close(sock_fd);
    return 0;
}
