#include <signal.h>

#include "../include/common.h"
#include "../include/driver.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
    return 1;
  }

  pid_t my_pid = getpid();
  char *fifo_name = get_driver_fifo_name(my_pid);

  int fifo_fd = open_fifo(fifo_name, O_RDONLY | O_NONBLOCK);
  if (fifo_fd < 0) {
    perror("Failed to open FIFO");
    free(fifo_name);
    return 1;
  }

  printf("Driver %d started\n", my_pid);

  run_driver_loop(fifo_fd);

  driver_cleanup(fifo_fd, fifo_name);
  free(fifo_name);
  printf("Driver %d exited\n", my_pid);
  return 0;
}