#include "cli.h"

#include <sys/wait.h>

#include "common.h"

pid_t drivers[MAX_DRIVERS];
int drivers_count = 0;

typedef struct {
  pid_t pid;
  int fifo_fd;
  int response_fd;
} driver_connection_t;

driver_connection_t connections[MAX_DRIVERS];
int connections_count = 0;

int get_connection_fd(pid_t pid, int *fifo_fd, int *response_fd) {
  for (int i = 0; i < connections_count; i++) {
    if (connections[i].pid == pid) {
      *fifo_fd = connections[i].fifo_fd;
      *response_fd = connections[i].response_fd;
      return 1;
    }
  }
  return 0;
}

int add_connection(pid_t pid, int fifo_fd, int response_fd) {
  for (int i = 0; i < connections_count; i++) {
    if (connections[i].pid == pid) {
      close(connections[i].fifo_fd);
      close(connections[i].response_fd);
      connections[i].fifo_fd = fifo_fd;
      connections[i].response_fd = response_fd;
      return 1;
    }
  }

  if (connections_count < MAX_DRIVERS) {
    connections[connections_count].pid = pid;
    connections[connections_count].fifo_fd = fifo_fd;
    connections[connections_count].response_fd = response_fd;
    connections_count++;
    return 1;
  }
  return 0;
}

void close_connection(pid_t pid) {
  for (int i = 0; i < connections_count; i++) {
    if (connections[i].pid == pid) {
      close(connections[i].fifo_fd);
      close(connections[i].response_fd);
      for (int j = i; j < connections_count - 1; j++) {
        connections[j] = connections[j + 1];
      }
      connections_count--;
      break;
    }
  }
}

void add_driver(pid_t pid) {
  for (int i = 0; i < drivers_count; i++) {
    if (drivers[i] == pid) {
      return;
    }
  }

  if (drivers_count < MAX_DRIVERS) {
    drivers[drivers_count++] = pid;

    char *fifo_name = get_driver_fifo_name(pid);
    int fifo_fd = open_fifo(fifo_name, O_WRONLY);
    free(fifo_name);

    char *response_fifo_name = get_driver_response_fifo_name(pid);
    int response_fd = open_fifo(response_fifo_name, O_RDONLY | O_NONBLOCK);
    free(response_fifo_name);

    if (fifo_fd >= 0 && response_fd >= 0) {
      add_connection(pid, fifo_fd, response_fd);
    } else {
      if (fifo_fd >= 0) close(fifo_fd);
      if (response_fd >= 0) close(response_fd);
    }
  }
}

void create_driver(void) {
  pid_t pid = fork();

  if (pid == 0) {
    char driver_pid_str[20];
    snprintf(driver_pid_str, sizeof(driver_pid_str), "%d", getpid());
    execl("./driver", "driver", driver_pid_str, NULL);
    perror("execl failed");
    exit(1);
  } else if (pid > 0) {
    char *fifo_name = get_driver_fifo_name(pid);
    if (create_fifo(fifo_name) == 0) {
      add_driver(pid);
      printf("Driver created with PID: %d\n", pid);
    } else {
      printf("Failed to create FIFO for driver %d\n", pid);
    }
    free(fifo_name);
  } else {
    perror("fork failed");
  }
}

void send_task(pid_t pid, int task_timer) {
  int fifo_fd, response_fd;

  if (!get_connection_fd(pid, &fifo_fd, &response_fd)) {
    printf("Driver %d not available\n", pid);
    return;
  }

  Command cmd = {CMD_SEND_TASK, task_timer};

  struct pollfd pfd = {fifo_fd, POLLOUT, 0};
  if (poll(&pfd, 1, 1000) > 0 && (pfd.revents & POLLOUT)) {
    if (write(fifo_fd, &cmd, sizeof(cmd)) == sizeof(cmd)) {
      printf("Task sent to driver %d for %d seconds\n", pid, task_timer);

      Response response;
      struct pollfd response_pfd = {response_fd, POLLIN, 0};
      if (poll(&response_pfd, 1, 1000) > 0 && (response_pfd.revents & POLLIN)) {
        if (read(response_fd, &response, sizeof(response)) > 0) {
          printf("Driver %d: %s", pid, response.response);
        }
      }
    } else {
      printf("Failed to send task to driver %d\n", pid);
    }
  } else {
    printf("Driver %d not responding\n", pid);
  }
}

void get_status(pid_t pid) {
  int fifo_fd, response_fd;

  if (!get_connection_fd(pid, &fifo_fd, &response_fd)) {
    printf("Driver %d not available\n", pid);
    return;
  }

  Command cmd = {CMD_GET_STATUS, 0};

  struct pollfd pfd = {fifo_fd, POLLOUT, 0};
  if (poll(&pfd, 1, 1000) > 0 && (pfd.revents & POLLOUT)) {
    if (write(fifo_fd, &cmd, sizeof(cmd)) == sizeof(cmd)) {
      Response response;
      struct pollfd response_pfd = {response_fd, POLLIN, 0};
      if (poll(&response_pfd, 1, 1000) > 0 && (response_pfd.revents & POLLIN)) {
        if (read(response_fd, &response, sizeof(response)) > 0) {
          printf("Driver %d: %s", pid, response.response);
        }
      }
    } else {
      printf("Failed to get status from driver %d\n", pid);
    }
  } else {
    printf("Driver %d not responding\n", pid);
  }
}

void get_drivers(void) {
  printf("PID\t\tStatus\t\tRemaining Time\n");
  printf("--------------------------------------------\n");

  int active_count = 0;

  for (int i = 0; i < drivers_count; i++) {
    pid_t pid = drivers[i];
    int fifo_fd, response_fd;

    if (!get_connection_fd(pid, &fifo_fd, &response_fd)) {
      printf("%d\t\tUnavailable\t-\n", pid);
      continue;
    }

    Command cmd = {CMD_GET_STATUS, 0};

    struct pollfd pfd = {fifo_fd, POLLOUT, 0};
    if (poll(&pfd, 1, 500) > 0 && (pfd.revents & POLLOUT)) {
      if (write(fifo_fd, &cmd, sizeof(cmd)) == sizeof(cmd)) {
        Response response;
        struct pollfd response_pfd = {response_fd, POLLIN, 0};

        if (poll(&response_pfd, 1, 500) > 0 &&
            (response_pfd.revents & POLLIN)) {
          if (read(response_fd, &response, sizeof(response)) > 0) {
            response.response[strcspn(response.response, "\n")] = 0;

            if (strncmp(response.response, "Busy", 4) == 0) {
              char *time_str = response.response + 5;
              printf("%d\t\tBusy\t\t%s seconds\n", pid, time_str);
            } else {
              printf("%d\t\tAvailable\t-\n", pid);
            }
            active_count++;
          }
        } else {
          printf("%d\t\tNo response\t-\n", pid);
        }
      } else {
        printf("%d\t\tWrite error\t-\n", pid);
      }
    } else {
      printf("%d\t\tNot responding\t-\n", pid);
    }
  }

  if (active_count == 0 && drivers_count > 0) {
    printf("No active drivers responding\n");
  } else if (drivers_count == 0) {
    printf("No drivers created\n");
  }
}

void cleanup_drivers(void) {
  for (int i = 0; i < drivers_count; i++) {
    if (is_process_alive(drivers[i])) {
      int fifo_fd, response_fd;
      if (get_connection_fd(drivers[i], &fifo_fd, &response_fd)) {
        Command cmd = {CMD_EXIT, 0};
        write(fifo_fd, &cmd, sizeof(cmd));
        usleep(100000);
      }
      kill(drivers[i], SIGTERM);
      waitpid(drivers[i], NULL, 0);
    }

    close_connection(drivers[i]);

    char *fifo_name = get_driver_fifo_name(drivers[i]);
    char *response_fifo_name = get_driver_response_fifo_name(drivers[i]);
    unlink(fifo_name);
    unlink(response_fifo_name);
    free(fifo_name);
    free(response_fifo_name);
  }
  drivers_count = 0;
  connections_count = 0;
}