#ifndef DRIVER_H
#define DRIVER_H

#include "common.h"

extern volatile sig_atomic_t running;

void driver_cleanup(int fifo_fd, int response_fd, const char *fifo_name,
                    const char *response_fifo_name);
int process_command(Command *cmd, int *status, int *task_timer,
                    time_t *task_end_time, int response_fd);
void signal_handler(int sig);
int run_driver_loop(int fifo_fd, int response_fd);

#endif