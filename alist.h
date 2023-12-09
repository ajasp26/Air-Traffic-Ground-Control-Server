#ifndef _ALIST_H
#define _ALIST_H

#include <pthread.h>

// The array list type

#define DEF_CAPACITY 10

typedef struct {
    void **data;   // Array of pointers to arraylist items
    int capacity;  // How big is the data array
    int in_use;    // How many items are in use (items 0..in_use-1)
    void (*dfree)(void *data); // Data destructor/freer
    pthread_rwlock_t lock;
} alist;

// Function prototypes

void alist_init(alist *a, void (*data_free)(void *data));
void alist_clear(alist *a);
int alist_is_empty(alist *a);
int alist_size(alist *a);
void *alist_get(alist *a, int index);
void alist_add(alist *a, void *val);
void alist_set(alist *a, int index, void *newval);
void alist_remove(alist *a, int index);
void alist_destroy(alist *a);

#endif  // _ALIST_H