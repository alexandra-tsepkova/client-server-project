#include "libs.h"

char *get_path(char *key, char **table) {
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

struct context{
    pthread_mutex_t *mutex;
    char **table;
    int sock_fd;
};

void *routine(void* context) {
    int sock_fd = ((struct context*)context)->sock_fd;
    char **table = ((struct context*)context)->table;
    pthread_mutex_t *mutex = ((struct context*)context)->mutex;

    while (1) {
        struct sockaddr_in rec_addr;
        char *ppid = calloc(PPID_SIZE, 1);
        char *buf = calloc(MAX_MESSAGE_SIZE, 1);
        int addr_size = sizeof(rec_addr);
        int res = recvfrom(sock_fd, buf, MAX_MESSAGE_SIZE, 0, (struct sockaddr * restrict)&rec_addr, (socklen_t * restrict)&addr_size);
        if (res < 0) {
            perror("Can't read message\n");
            exit (-1);
        }
        char *client_ip = to_addr(&rec_addr);
        strncpy(ppid, buf, PPID_SIZE);
        char *key = calloc(strlen(client_ip) + PPID_SIZE, sizeof(char));
        strcpy(key, client_ip);
        strcat(key, "-");
        strcat(key, ppid);

        printf("%s\n", key);
        puts(buf + PPID_SIZE);

        pthread_mutex_lock(mutex);
        char *path = get_path(key, table);
        pthread_mutex_unlock(mutex);

        char *resp = calloc(MAX_MESSAGE_SIZE, 1);
        memset((void *) resp, 0, MAX_MESSAGE_SIZE);

        if(strncmp(buf + PPID_SIZE, "cd", 2) == 0) {
            char *new_path = calloc(1, PATH_MAX);
            char *cur_path = calloc(1, PATH_MAX);
            strcpy(new_path, (buf + PPID_SIZE + 3));
            fflush(stdout);
            if(chdir(path) != 0){
                perror("Can't go to this directory\n");
                exit (-1);
            }
            if(chdir(new_path) != 0){
                perror("Can't go to this directory\n");
            }
            getcwd(cur_path, PATH_MAX);
            printf("new path: %s, old_path: %s\n", cur_path, path);
            sprintf(resp, "new path: %s, old_path: %s\n", cur_path, path);
            strcpy(path, cur_path);
            free(new_path);

        } else if(strncmp(buf + PPID_SIZE, "ls", 2) == 0) {
            DIR *dir = NULL;
            struct dirent *this_dir;
            dir = opendir(path);
            if(!dir) {
                perror("Can't read directory\n");
                exit(-1);
            }
            while((this_dir = readdir(dir)) != NULL) {
                char dirname[1024];
                sprintf(dirname, "%s  ", this_dir->d_name);
                strcat(resp, dirname);
            }
            printf("%s", resp);
            if(closedir(dir) != 0){
                perror("Can't close directory\n");
                exit(-1);
            }
        } else if (strncmp(buf + PPID_SIZE, "shell", 5) == 0) {
            int fd = open("/dev/ptmx", O_RDWR|O_NOCTTY);
            grantpt(fd);
            unlockpt(fd);
            char *pts_path = ptsname(fd);
            if(pts_path == NULL) {
                perror("Error in pts path\n");
            }

            int client_pid = fork();
            if(client_pid < 0){
                perror("Can't fork\n");
                exit(-1);
            } else if (client_pid != 0) {
                // Parent
                setsid();
                strcat(buf, "\n");
                write(fd, buf + PPID_SIZE + 6, strlen(buf + PPID_SIZE + 6));
                sleep(1);
                read(fd, resp, MAX_MESSAGE_SIZE);

                kill(client_pid, 2);
            } else {
                // Child
                setsid();
                int resfd = open(pts_path, O_RDWR);
                dup2(resfd, STDIN_FILENO);
                dup2(resfd, STDOUT_FILENO);
                dup2(resfd, STDERR_FILENO);

                execlp("bash", "bash", (char *) NULL);
            }


        }
        int resp_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
        rec_addr.sin_port = htons(RECV_PORT);
        sendto(resp_sock_fd, resp, strlen(resp), 0, (struct sockaddr*)&rec_addr, sizeof(rec_addr));

        free(buf);
        free(key);
    }
    return NULL;
}

int daemon_exec(int argc, char **argv) {
    char *cur_dir = calloc(PATH_MAX, sizeof(char));
    strcat(getcwd(cur_dir, PATH_MAX), "/log_server.txt");
    puts(cur_dir);
    log_init(cur_dir);

    pr_info("Server started\n");

    char **table = calloc(TABLE_SIZE * 2, sizeof(char*));

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_fd < 0) {
        pr_err("Can't create socket\n");
        return -1;
    }
    struct in_addr addr;
    addr.s_addr = inet_addr("0.0.0.0");
    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(PORT);
    sock_addr.sin_addr = addr;

    if (bind(sock_fd, (const struct sockaddr*)(&sock_addr), (socklen_t)sizeof(sock_addr)) < 0) {
        pr_err("Can't assign address to a socket\n");
        close(sock_fd);
        exit(-1);
    }

    pthread_mutex_t mutex;
    if(pthread_mutex_init(&mutex, NULL) != 0){
        pr_err("Can't create mutex\n");
        exit(-1);
    }

    struct context variables = {&mutex, table, sock_fd};

    pthread_t thread_id[MAX_THREADS];
    for(int i = 0; i < MAX_THREADS; ++i) {
        if(pthread_create((pthread_t *)(thread_id + i), NULL, &routine, (void*)&variables) != 0){
            pr_err("Can't create thread\n");
            exit(-1);
        }
    }


    int b_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(b_sock_fd < 0) {
        pr_err("Can't create socket\n");
        return -1;
    }

    struct sockaddr_in b_sock_addr;
    int b_addr_len = sizeof(struct sockaddr_in);

    memset((void*)&b_sock_addr, 0, b_addr_len);
    b_sock_addr.sin_family = AF_INET;
    b_sock_addr.sin_port = htons(B_PORT); //!!
    b_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(b_sock_fd, (const struct sockaddr*)(&b_sock_addr), b_addr_len) < 0) {
        pr_err("Can't assign address to a socket (broadcast)\n");
        close(b_sock_fd);
        exit(-1);
    }

    while (1) {
        struct sockaddr_in b_rec_addr;
        char *b_buf = calloc(MAX_MESSAGE_SIZE, 1);
        int b_addr_size = sizeof(b_rec_addr);
        int b;
        b = recvfrom(b_sock_fd, b_buf, MAX_MESSAGE_SIZE, 0, (struct sockaddr *restrict) &b_rec_addr,
                     (socklen_t *restrict) &b_addr_size);
        if (b < 0) {
            pr_err("Can't read message\n");
            exit(-1);
        }

        if(strcmp(b_buf, "Is anybody here?") == 0){
            printf("\nBroadcast message received; sending response\n");
            if(sendto(b_sock_fd, "Response message", strlen("Response message"), 0, (const struct sockaddr*) &b_rec_addr,
                    b_addr_len) < 0){
                pr_err("Can't send response message to client\n");
                exit(-1);
            }
        } else if(strncmp(b_buf + PPID_SIZE, "exit", 4) == 0) {
            remove("/var/run/client_server.pid");
            break;
        }
    }

    for(int i = 0; i < MAX_THREADS; ++i)    {
        pthread_cancel(thread_id[i]);
    }

    close(sock_fd);
    pr_info("Server disconnected\n");
    return 0;
}

int main(int argc, char **argv) {
    if(access( "/var/run/client_server.pid", F_OK) != -1)
    {
        printf("Daemon already running\n");
        exit(0);
    }
    else
    {
        int pid = fork();
        if(pid < 0) {
            perror("Can't create daemon!\n");
            exit(-1);
        }
        if(pid != 0) {
            char *child_id = calloc(PPID_SIZE, sizeof(char));
            int pid_fd = open("/var/run/client_server.pid", O_WRONLY | O_CREAT, 0666);
            if(pid_fd < 0) {
                perror("Can't create .pid file\n");
                exit(-1);
            }
            sprintf(child_id, "%d", pid);
            write(pid_fd, child_id, PPID_SIZE);
            exit(0);
        }

        setsid();
        int devNull = open("/dev/null", O_WRONLY);
        dup2(devNull, STDIN_FILENO);
        dup2(devNull, STDOUT_FILENO);
        dup2(devNull, STDERR_FILENO);
        daemon_exec(argc, argv);
    }
    return 0;
}