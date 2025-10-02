#include "common.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int create_fifo(const char *fifo_name) {
  if (mkfifo(fifo_name, 0666) == -1 && errno != EEXIST) {
    perror("mkfifo failed");
    return -1;
  }
  return 0;
}

int open_fifo(const char *fifo_name, int mode) {
  int fd;
  int retry_count = 0;
  while ((fd = open(fifo_name, mode)) == -1) {
    if (errno != EINTR) {
      if (retry_count++ < 3) {
        sleep(1);
        continue;
      }
      perror("open FIFO failed");
      return -1;
    }
  }
  return fd;
}

char *get_driver_fifo_name(pid_t pid) {
  char *fifo_name = malloc(FIFO_NAME_SIZE);
  snprintf(fifo_name, FIFO_NAME_SIZE, "/tmp/taxi_driver_%d", pid);
  return fifo_name;
}

char *get_driver_response_fifo_name(pid_t pid) {
  char *fifo_name = malloc(FIFO_NAME_SIZE);
  snprintf(fifo_name, FIFO_NAME_SIZE, "/tmp/taxi_driver_response_%d", pid);
  return fifo_name;
}

int is_process_alive(pid_t pid) { return kill(pid, 0) == 0; }