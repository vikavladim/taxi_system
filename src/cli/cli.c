#include "cli.h"

#include <sys/wait.h>

#include "common.h"

driver_list_t drivers_list;

void remove_dead_drivers(void) {
  driver_node_t *current = drivers_list.head;
  driver_node_t *prev = NULL;

  while (current != NULL) {
    driver_node_t *next = current->next;

    if (!is_process_alive(current->info.pid)) {
      printf("Driver %d is dead, removing...\n", current->info.pid);

      if (prev == NULL) {
        drivers_list.head = next;
      } else {
        prev->next = next;
      }

      unlink(current->info.fifo_name);
      free(current);
      drivers_list.count--;
    } else {
      prev = current;
    }

    current = next;
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
    driver_node_t *driver = driver_list_add(&drivers_list, pid);
    if (driver != NULL) {
      driver->info.status = 0;
      driver->info.task_timer = 0;

      if (create_fifo(driver->info.fifo_name) == 0) {
        printf("Driver created with PID: %d\n", pid);
      } else {
        printf("Failed to create FIFO for driver %d\n", pid);
        driver_list_remove(&drivers_list, pid);
      }
    } else {
      printf("Failed to add driver to list\n");
    }
  } else {
    perror("fork failed");
  }
}

void send_task(pid_t pid, int task_timer) {
  remove_dead_drivers();

  driver_node_t *driver = driver_list_find(&drivers_list, pid);
  if (driver == NULL) {
    printf("Driver with PID %d not found\n", pid);
    return;
  }

  if (driver->info.status == 1) {
    int remaining = driver->info.task_end_time - time(NULL);
    if (remaining > 0) {
      printf("Busy %d\n", remaining);
    } else {
      driver->info.status = 0;
      Command status_cmd = {CMD_GET_STATUS, 0};
      int fifo_fd = open_fifo(driver->info.fifo_name, O_WRONLY | O_NONBLOCK);
      if (fifo_fd >= 0) {
        write(fifo_fd, &status_cmd, sizeof(status_cmd));
        close(fifo_fd);
      }
    }
    return;
  }

  int fifo_fd = open_fifo(driver->info.fifo_name, O_WRONLY | O_NONBLOCK);
  if (fifo_fd < 0) {
    printf("Failed to communicate with driver %d\n", pid);
    return;
  }

  Command cmd = {CMD_SEND_TASK, task_timer};

  struct pollfd pfd = {fifo_fd, POLLOUT, 0};
  if (poll(&pfd, 1, 1000) > 0 && (pfd.revents & POLLOUT)) {
    if (write(fifo_fd, &cmd, sizeof(cmd)) == sizeof(cmd)) {
      driver->info.status = 1;
      driver->info.task_timer = task_timer;
      driver->info.task_end_time = time(NULL) + task_timer;
      printf("Task sent to driver %d for %d seconds\n", pid, task_timer);
    } else {
      printf("Failed to send task to driver %d\n", pid);
    }
  } else {
    printf("Driver %d not responding\n", pid);
  }

  close(fifo_fd);
}

void get_status(pid_t pid) {
  remove_dead_drivers();

  driver_node_t *driver = driver_list_find(&drivers_list, pid);
  if (driver == NULL) {
    printf("Driver with PID %d not found\n", pid);
    return;
  }

  if (driver->info.status == 1) {
    time_t current_time = time(NULL);
    if (current_time >= driver->info.task_end_time) {
      driver->info.status = 0;
      driver->info.task_timer = 0;
      printf("Available\n");
    } else {
      int remaining = driver->info.task_end_time - current_time;
      printf("Busy %d\n", remaining);
    }
  } else {
    printf("Available\n");
  }
}

void get_drivers(void) {
  remove_dead_drivers();
  driver_list_print(&drivers_list);
}

void cleanup_drivers(void) { driver_list_clear(&drivers_list); }