#ifndef CLI_H
#define CLI_H

#include <unistd.h>

#define MAX_DRIVERS 100

extern pid_t drivers[MAX_DRIVERS];
extern int drivers_count;

void create_driver(void);
void send_task(pid_t pid, int task_timer);
void get_status(pid_t pid);
void get_drivers(void);
void cleanup_drivers(void);

#endif