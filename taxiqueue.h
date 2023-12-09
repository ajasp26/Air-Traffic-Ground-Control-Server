#ifndef TAXIQUEUE_H
#define TAXIQUEUE_H

#include <pthread.h>

void taxiqueue_init();
void taxiqueue_add(const char *flight_id);
int taxiqueue_getpos(const char *flight_id);
char *taxiqueue_getahead(const char *flight_id);
void taxiqueue_inair(const char *flight_id);
void *taxiqueue_manager(void *arg);

#endif // TAXIQUEUE_H
