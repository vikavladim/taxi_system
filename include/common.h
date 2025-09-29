#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FIFO_NAME_SIZE 256
#define BUFFER_SIZE 1024

typedef enum { CMD_SEND_TASK, CMD_GET_STATUS, CMD_EXIT } CommandType;

typedef struct {
  CommandType type;
  int task_timer;
} Command;

typedef struct {
  pid_t pid;
  int status;
  int task_timer;
  time_t task_end_time;
  char fifo_name[FIFO_NAME_SIZE];
} DriverInfo;

int create_fifo(const char *fifo_name);
int open_fifo(const char *fifo_name, int mode);
char *get_driver_fifo_name(pid_t pid);
int is_process_alive(pid_t pid);

#endif