#include "driver_list.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"

void driver_list_init(driver_list_t *list) {
  list->head = NULL;
  list->count = 0;
}

driver_node_t *driver_list_find(driver_list_t *list, pid_t pid) {
  driver_node_t *current = list->head;
  while (current != NULL) {
    if (current->info.pid == pid) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

driver_node_t *driver_list_add(driver_list_t *list, pid_t pid) {
  driver_node_t *existing = driver_list_find(list, pid);
  if (existing != NULL) {
    return existing;
  }

  driver_node_t *new_node = malloc(sizeof(driver_node_t));
  if (!new_node) {
    return NULL;
  }

  new_node->info.pid = pid;
  new_node->info.status = 0;
  new_node->info.task_timer = 0;
  new_node->info.task_end_time = 0;

  char *fifo_name = get_driver_fifo_name(pid);
  strncpy(new_node->info.fifo_name, fifo_name, FIFO_NAME_SIZE);
  free(fifo_name);

  new_node->next = list->head;
  list->head = new_node;
  list->count++;

  return new_node;
}

int driver_list_remove(driver_list_t *list, pid_t pid) {
  driver_node_t *current = list->head;
  driver_node_t *prev = NULL;

  while (current != NULL) {
    if (current->info.pid == pid) {
      if (prev == NULL) {
        list->head = current->next;
      } else {
        prev->next = current->next;
      }

      unlink(current->info.fifo_name);
      free(current);
      list->count--;
      return 1;
    }
    prev = current;
    current = current->next;
  }
  return 0;
}

void driver_list_clear(driver_list_t *list) {
  driver_node_t *current = list->head;
  while (current != NULL) {
    driver_node_t *next = current->next;
    if (is_process_alive(current->info.pid)) {
      int fifo_fd = open_fifo(current->info.fifo_name, O_WRONLY | O_NONBLOCK);
      if (fifo_fd >= 0) {
        Command cmd = {CMD_EXIT, 0};
        write(fifo_fd, &cmd, sizeof(cmd));
        close(fifo_fd);
      }
      kill(current->info.pid, SIGTERM);
      waitpid(current->info.pid, NULL, 0);
    }
    unlink(current->info.fifo_name);
    free(current);
    current = next;
  }
  list->head = NULL;
  list->count = 0;
}

void driver_list_print(driver_list_t *list) {
  time_t current_time = time(NULL);

  printf("PID\t\tStatus\t\tRemaining Time\n");
  printf("--------------------------------------------\n");

  driver_node_t *current = list->head;
  while (current != NULL) {
    if (current->info.status == 1 &&
        current_time >= current->info.task_end_time) {
      current->info.status = 0;
      current->info.task_timer = 0;
    }

    printf("%d\t\t", current->info.pid);
    if (current->info.status == 1) {
      int remaining = current->info.task_end_time - current_time;
      printf("Busy\t\t%d seconds\n", remaining > 0 ? remaining : 0);
    } else {
      printf("Available\t-\n");
    }
    current = current->next;
  }

  if (list->count == 0) {
    printf("No drivers available\n");
  }
}