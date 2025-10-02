#include <signal.h>

#include "common.h"
#include "driver.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
    return 1;
  }

  pid_t my_pid = getpid();
  char *fifo_name = get_driver_fifo_name(my_pid);
  char *response_fifo_name = get_driver_response_fifo_name(my_pid);

  create_fifo(fifo_name);
  create_fifo(response_fifo_name);

  int fifo_fd = open_fifo(fifo_name, O_RDONLY | O_NONBLOCK);
  if (fifo_fd < 0) {
    perror("Failed to open command FIFO");
    free(fifo_name);
    free(response_fifo_name);
    return 1;
  }

  int response_fd = open_fifo(response_fifo_name, O_WRONLY);
  if (response_fd < 0) {
    perror("Failed to open response FIFO");
    close(fifo_fd);
    free(fifo_name);
    free(response_fifo_name);
    return 1;
  }

  printf("Driver %d started\n", my_pid);

  run_driver_loop(fifo_fd, response_fd);

  driver_cleanup(fifo_fd, response_fd, fifo_name, response_fifo_name);
  free(fifo_name);
  free(response_fifo_name);
  printf("Driver %d exited\n", my_pid);
  return 0;
}