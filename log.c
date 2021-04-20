#include "log.h"

static int pos = 0;
static char buf[BUFSZ];
static int logfd = -1;

void log_init(char* path) {
    logfd = open(path, O_WRONLY | O_APPEND);
    if(logfd == (-1)) {
        exit(-1);
    }
}


void make_log_entry(int log_level) {
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    pos += sprintf(buf, "%d: [PID] %d  [Date] %s  ", log_level, getpid(), asctime (timeinfo));

    if(pos > BUFSZ) { //so unlikely to happen
        printf("out of bounds: buf\n");
    }

}

int pr_log_level (int log_level, char *fmt, ...){
    memset(buf, 0, BUFSZ);

    make_log_entry(log_level);
    va_list params;
    va_start(params, fmt);

    vsnprintf(buf + pos, BUFSZ - pos, fmt, params);

    va_end(params);

    int n = write(logfd, buf, strlen(buf));
    if (n != strlen(buf)) {
        perror("Can't write into log file:\n");
    }
    pos = 0;
    return 0;
}







