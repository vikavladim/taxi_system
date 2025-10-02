#include "driver.h"

#include <poll.h>
#include <signal.h>

#include "common.h"

volatile sig_atomic_t running = 1;

void driver_cleanup(int fifo_fd, int response_fd, const char *fifo_name,
                    const char *response_fifo_name) {
  if (fifo_fd >= 0) {
    close(fifo_fd);
  }
  if (response_fd >= 0) {
    close(response_fd);
  }
  if (fifo_name) {
    unlink(fifo_name);
  }
  if (response_fifo_name) {
    unlink(response_fifo_name);
  }
}

int process_command(Command *cmd, int *status, int *task_timer,
                    time_t *task_end_time, int response_fd) {
  time_t current_time = time(NULL);
  Response response;

  switch (cmd->type) {
    case CMD_SEND_TASK:
      if (*status == 1) {
        int remaining = *task_end_time - current_time;
        if (remaining < 0) remaining = 0;
        snprintf(response.response, sizeof(response.response), "Busy %d\n",
                 remaining);
      } else {
        *status = 1;
        *task_timer = cmd->task_timer;
        *task_end_time = current_time + cmd->task_timer;
        snprintf(response.response, sizeof(response.response),
                 "Task received: %d seconds\n", cmd->task_timer);
      }
      break;

    case CMD_GET_STATUS:
      if (*status == 1) {
        if (current_time >= *task_end_time) {
          *status = 0;
          *task_timer = 0;
          snprintf(response.response, sizeof(response.response), "Available\n");
        } else {
          int remaining = *task_end_time - current_time;
          snprintf(response.response, sizeof(response.response), "Busy %d\n",
                   remaining);
        }
      } else {
        snprintf(response.response, sizeof(response.response), "Available\n");
      }
      break;

    case CMD_EXIT:
      return -1;

    default:
      snprintf(response.response, sizeof(response.response),
               "Unknown command\n");
      break;
  }

  if (response_fd >= 0) {
    write(response_fd, &response, sizeof(response));
  }

  return 0;
}

void signal_handler(int sig) {
  (void)sig;
  running = 0;
}

int run_driver_loop(int fifo_fd, int response_fd) {
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
        int result = process_command(&cmd, &status, &task_timer, &task_end_time,
                                     response_fd);
        if (result == -1) {
          break;
        }
      }
    } else if (ret < 0 && errno != EINTR) {
      perror("poll error");
      break;
    }

    if (status == 1) {
      time_t current_time = time(NULL);
      if (current_time >= task_end_time) {
        status = 0;
        task_timer = 0;

        Response response;
        snprintf(response.response, sizeof(response.response),
                 "Task completed\n");
        if (response_fd >= 0) {
          write(response_fd, &response, sizeof(response));
        }
        printf("Task completed\n");
      }
    }
  }

  return 0;
}