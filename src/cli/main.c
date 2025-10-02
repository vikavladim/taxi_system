#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "common.h"

void signal_handler(int sig) {
  (void)sig;
  printf("\nShutting down...\n");
  cleanup_drivers();
  exit(0);
}

int main() {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  atexit(cleanup_drivers);

  char command[BUFFER_SIZE];
  pid_t pid;
  int task_timer;

  printf("Taxi Management System CLI\n");
  printf("Available commands:\n");
  printf("  create_driver\n");
  printf("  send_task <pid> <task_timer>\n");
  printf("  get_status <pid>\n");
  printf("  get_drivers\n");
  printf("  exit\n\n");

  while (1) {
    printf("> ");
    fflush(stdout);

    if (fgets(command, BUFFER_SIZE, stdin) == NULL) {
      break;
    }

    command[strcspn(command, "\n")] = 0;

    if (strcmp(command, "exit") == 0) {
      break;
    } else if (strcmp(command, "create_driver") == 0) {
      create_driver();
    } else if (strcmp(command, "get_drivers") == 0) {
      get_drivers();
    } else if (sscanf(command, "send_task %d %d", &pid, &task_timer) == 2) {
      send_task(pid, task_timer);
    } else if (sscanf(command, "get_status %d", &pid) == 1) {
      get_status(pid);
    } else {
      printf("Unknown command: %s\n", command);
    }
  }

  return 0;
}