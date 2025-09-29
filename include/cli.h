#ifndef CLI_H
#define CLI_H

#include "driver_list.h"

extern driver_list_t drivers_list;

void remove_dead_drivers(void);
void create_driver(void);
void send_task(pid_t pid, int task_timer);
void get_status(pid_t pid);
void get_drivers(void);
void cleanup_drivers(void);

#endif