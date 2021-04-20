#ifndef LOG_H
#define LOG_H

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define LOG_ERR 1
#define LOG_WARN 2
#define LOG_INFO 3
#define BUFSZ 4096

void log_init (char *path);
int pr_log_level(int log_level, char *fmt, ...);

#define pr_err(fmt, ...) pr_log_level(LOG_ERR, "%s:%d  " fmt, __FILE__, __LINE__, ##__VA_ARGS__);
#define pr_warn(fmt, ...) pr_log_level(LOG_WARN, "%s:%d  " fmt, __FILE__, __LINE__, ##__VA_ARGS__);
#define pr_info(fmt, ...) pr_log_level(LOG_INFO, "%s:%d  " fmt, __FILE__, __LINE__, ##__VA_ARGS__);

#endif
