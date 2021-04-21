#include "libs.h"

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
                sprintf(resp, "Can't go to this directory: %s", path);
                pr_info("Can't go to this directory %s\n", path);
            }
            if(chdir(new_path) != 0){
                sprintf(resp, "Can't go to this directory: %s", new_path);
                pr_info("Can't go to this directory %s\n", path);
            }
            else {
                getcwd(cur_path, PATH_MAX);
                sprintf(resp, "new path: %s, old_path: %s", cur_path, path);
                strcpy(path, cur_path);
            }
            free(new_path);

        } else if(strncmp(buf + PPID_SIZE, "ls", 2) == 0) {
            DIR *dir = NULL;
            struct dirent *this_dir;
            dir = opendir(path);
            if(!dir) {
                pr_warn("Can't read directory with path %s\n", path);
                strcat(resp, "Can't read directory\n");
            }
            else {
                while ((this_dir = readdir(dir)) != NULL) {
                    char dirname[1024];
                    sprintf(dirname, "%s  ", this_dir->d_name);
                    strcat(resp, dirname);
                }
                if (closedir(dir) != 0) {
                    pr_warn("Can't close directory with path %s\n", path);
                }
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


void daemon_exec() {
    char *cur_dir = calloc(PATH_MAX, sizeof(char)); //initializing log in current directory
    strcat(getcwd(cur_dir, PATH_MAX), "/log_server.txt");
    puts(cur_dir);
    log_init(cur_dir);

    pr_info("Server started\n");

    char **table = calloc(TABLE_SIZE * 2, sizeof(char*));

    int sock_fd = create_socket(); //creating socket for messages

    struct sockaddr_in sock_addr = create_addr("0.0.0.0", PORT);

    bind_socket(sock_fd, sock_addr);


    pthread_mutex_t mutex;
    if(pthread_mutex_init(&mutex, NULL) != 0){
        pr_err("Can't create mutex\n");
        my_exit();
    }

    struct context variables = {&mutex, table, sock_fd};

    pthread_t thread_id[MAX_THREADS];
    for(int i = 0; i < MAX_THREADS; ++i) {
        if(pthread_create((pthread_t *)(thread_id + i), NULL, &routine, (void*)&variables) != 0){
            pr_err("Can't create thread\n");
            my_exit();
        }
    }

    int b_sock_fd = create_socket(); //creating socket for broadcast messages

    int b_addr_len = sizeof(struct sockaddr_in);
//    memset((void*)&b_sock_addr, 0, b_addr_len);
    struct sockaddr_in b_sock_addr = create_addr("0.0.0.0", B_PORT);

    bind_socket(b_sock_fd, b_sock_addr);

    while (1) { //on initial thread on broadcast socket
        struct sockaddr_in b_rec_addr; //address to send responses to
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
            pr_info("Broadcast message received; sending response\n");
            if(sendto(b_sock_fd, "Response message", strlen("Response message"), 0, (const struct sockaddr*) &b_rec_addr,
                      b_addr_len) < 0){
                pr_err("Can't send response message to client\n");
                exit(-1);
            }
        } else if(strncmp(b_buf + PPID_SIZE, "exit", 4) == 0) { //shutting down server without errors
            break;
        }
        else {
            pr_info("Received message %s on broadcast socket\n", b_buf);
        }
    }

    for(int i = 0; i < MAX_THREADS; ++i)    {
        pthread_cancel(thread_id[i]);
    }

    close(sock_fd);
    close(b_sock_fd);
    remove("/var/run/client_server.pid");
    pr_info("Server disconnected\n");
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
        daemon_exec();
    }
    return 0;
}