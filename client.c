#include "libs.h"
#include "udp_lib.h"

int main(int argc, char **argv) {
    int ppid = getppid();

    int sock_fd = create_socket();
    int server_len = sizeof(struct sockaddr_in);
    struct sockaddr_in server_addr = create_addr("127.0.0.1", PORT);

    if(argc < 2) {
        printf("Not enough arguments\n");
        exit(0);
    }

    if(strcmp("find", argv[1]) == 0) { //this is the place where we use broadcast
        int b_sock_fd = create_socket();

        int yes = 1;
        int res = setsockopt(b_sock_fd, SOL_SOCKET, SO_BROADCAST, (char *) &yes, sizeof(yes));
        if (res == -1) {
            printf("Setsockopt error\n");
            my_exit();
        }

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        res = setsockopt(b_sock_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout));
        if (res == -1) {
            printf("Setsockopt error\n");
            my_exit();
        }


        int b_addr_len = sizeof(struct sockaddr_in);
        struct sockaddr_in b_sock_addr = create_addr("255.255.255.255", B_PORT);
//        memset((void *) &b_sock_addr, 0, b_addr_len);


        char *b_buf = calloc(MAX_MESSAGE_SIZE, 1);
        strcpy(b_buf, "Is anybody here?");


        int b_sent = sendto(b_sock_fd, "Is anybody here?", strlen("Is anybody here?"), 0,
                            (const struct sockaddr *) &b_sock_addr, b_addr_len);
        if (b_sent < 0) {
            printf("Error while sending (broadcast)\n");
            my_exit();
        }

        int any_server_found = 0;
        while (1) {
            struct sockaddr_in b_rec_addr;
            char *rec_buf = calloc(MAX_MESSAGE_SIZE, 1);
            int b_addr_size = sizeof(b_rec_addr);
            int b = recvfrom(b_sock_fd, rec_buf, MAX_MESSAGE_SIZE, 0, (struct sockaddr *restrict) &b_rec_addr,
                         (socklen_t *restrict) &b_addr_size);
            if (b < 0 && any_server_found == 0) {
                printf("No server found\n");
                exit(0);
            } else if (b < 0 && any_server_found == 1) {
                exit(0);
            } else {
                printf("Server found: %s\n", to_addr(&b_rec_addr));
                any_server_found = 1;
            }
        }
    } else { //and this is for everything but broadcast
        char *buf = calloc(MAX_MESSAGE_SIZE, 1);
        sprintf(buf, "%05d", ppid);
        strcat(buf, argv[1]);
        if(argv[2]) {
            strcat(buf, " ");
            strcat(buf, argv[2]);
        }

        int is_exiting = 0;
        if(strncmp(buf + PPID_SIZE, "exit", 4) == 0) {
            server_addr.sin_port = htons(B_PORT);
            is_exiting = 1;
        }

        int sent = sendto(sock_fd, buf, strlen(buf), 0, (struct sockaddr*)&server_addr, server_len);
        if(sent < 0) {
            printf("Error while sending\n");
            my_exit();
        }

        if(is_exiting == 1) {
            printf("Shutting down\n");
            exit(1);
        }

        int recv_sock_fd = create_socket();
        struct sockaddr_in sock_addr = create_addr("0.0.0.0", RECV_PORT);
        bind_socket(recv_sock_fd, sock_addr);


        struct sockaddr_in received_addr;
        char *rec_buf = calloc(MAX_MESSAGE_SIZE, 1);
        int recv_addr_size = sizeof(received_addr);
        int b = recvfrom(recv_sock_fd, rec_buf, MAX_MESSAGE_SIZE, 0, (struct sockaddr *restrict) &received_addr,
                     (socklen_t *restrict) &recv_addr_size);
        if (b < 0) {
            printf("Can't read message\n");
            my_exit();
        }
        printf("%s\n", rec_buf);
        close(recv_sock_fd);
    }
    close(sock_fd);
    return 0;
}
