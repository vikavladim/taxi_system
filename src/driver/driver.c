#include "../include/driver.h"

#include <poll.h>
#include <signal.h>

#include "../include/common.h"

volatile sig_atomic_t running = 1;

void driver_cleanup(int fifo_fd, const char *fifo_name) {
  if (fifo_fd >= 0) {
    close(fifo_fd);
  }
  if (fifo_name) {
    unlink(fifo_name);
  }
}

int process_command(Command *cmd, int *status, int *task_timer,
                    time_t *task_end_time) {
  time_t current_time = time(NULL);

  switch (cmd->type) {
    case CMD_SEND_TASK:
      if (*status == 1) {
        int remaining = *task_end_time - current_time;
        printf("Busy %d\n", remaining > 0 ? remaining : 0);
        return 0;
      } else {
        *status = 1;
        *task_timer = cmd->task_timer;
        *task_end_time = current_time + cmd->task_timer;
        printf("Task received: %d seconds\n", cmd->task_timer);
        return 1;
      }

    case CMD_GET_STATUS:
      if (*status == 1) {
        if (current_time >= *task_end_time) {
          *status = 0;
          *task_timer = 0;
          printf("Available\n");
        } else {
          int remaining = *task_end_time - current_time;
          printf("Busy %d\n", remaining);
        }
      } else {
        printf("Available\n");
      }
      return 0;

    case CMD_EXIT:
      return -1;
  }
  return 0;
}

void signal_handler(int sig) {
  (void)sig;
  running = 0;
}

int run_driver_loop(int fifo_fd) {
  int status = 0;
  int task_timer = 0;
  time_t task_end_time = 0;

  struct pollfd fds[1];
  fds[0].fd = fifo_fd;
  fds[0].events = POLLIN;

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  while (running) {
    int ret = poll(fds, 1, 1000);

    if (ret > 0 && (fds[0].revents & POLLIN)) {
      Command cmd;
      ssize_t bytes_read = read(fifo_fd, &cmd, sizeof(cmd));

      if (bytes_read == sizeof(cmd)) {
        int result =
            process_command(&cmd, &status, &task_timer, &task_end_time);
        if (result == -1) {
          break;
        }
      }
    } else if (ret < 0 && errno != EINTR) {
      perror("poll error");
      break;
    }

    if (status == 1 && time(NULL) >= task_end_time) {
      status = 0;
      task_timer = 0;
      printf("Task completed\n");
    }
  }

  return 0;
}