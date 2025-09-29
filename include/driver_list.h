#ifndef DRIVER_LIST_H
#define DRIVER_LIST_H

#include "common.h"

typedef struct driver_node {
  DriverInfo info;
  struct driver_node *next;
} driver_node_t;

typedef struct {
  driver_node_t *head;
  int count;
} driver_list_t;

void driver_list_init(driver_list_t *list);
driver_node_t *driver_list_find(driver_list_t *list, pid_t pid);
driver_node_t *driver_list_add(driver_list_t *list, pid_t pid);
int driver_list_remove(driver_list_t *list, pid_t pid);
void driver_list_clear(driver_list_t *list);
void driver_list_print(driver_list_t *list);

#endif