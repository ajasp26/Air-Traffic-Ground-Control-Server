// CSC 362 - C implementation of an arraylist - for Fall 2023 project

// This version is thread-safe and items are generic

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "alist.h"

/***************************************************************************
 * alist_init initializes an array list to empty and with the default
 * capacity.
 */
void alist_init(alist *a, void (*data_free)(void *data) ) {
    if ((a->data=malloc(DEF_CAPACITY*sizeof(void *))) == NULL) {
        perror("alist_init");
        exit(1);
    }

    pthread_rwlock_init(&(a->lock), NULL);
    a->capacity = DEF_CAPACITY;
    a->in_use = 0;
    a->dfree = data_free;
}

/***************************************************************************
 * alist_clear resets the size of the array list to 0 (empties the alist).
 */
void alist_clear(alist *a) {
    pthread_rwlock_wrlock(&(a->lock));
    for (int i=0; i<a->in_use; i++) {
        a->dfree(a->data[i]);
    }

    a->in_use = 0;
    pthread_rwlock_unlock(&(a->lock));
}

/***************************************************************************
 * alist_is_empty returns true if and only if the array list is empty.
 */

int alist_is_empty(alist *a) {
    return (a->in_use == 0);
}

/***************************************************************************
 * alist_size returns the size of the array list
 */
int alist_size(alist *a) {
    return a->in_use;
}

/***************************************************************************
 * alist_get returns the value at array index "index", or NULL if this is
 * an invalid index.
 */
void *alist_get(alist *a, int index) {
    pthread_rwlock_rdlock(&(a->lock));
    if ((index < 0) || (index >= a->in_use)) {
        pthread_rwlock_unlock(&(a->lock));
        return NULL;
    }

    void *retval = a->data[index];
    pthread_rwlock_unlock(&(a->lock));
    return retval;
}

/***************************************************************************
 * alist_add appends a new value to the end of the array list.
 */
void alist_add(alist *a, void *val) {
    pthread_rwlock_wrlock(&(a->lock));
    if (a->in_use == a->capacity) {
        void *newdata = realloc(a->data, 2*a->capacity*sizeof(void *));
        if (newdata == NULL) {
            perror("alist_add - growing array");
            exit(1);
        }
        a->capacity = 2*a->capacity;
        a->data = newdata;
    }

    a->data[a->in_use++] = val;
    pthread_rwlock_unlock(&(a->lock));
}

/***************************************************************************
 * alist_set sets the index "index" item to value "val". If the
 * index/position doesn't exist in the list, then nothing happens (the
 * request is ignored).
 */
void alist_set(alist *a, int index, void *val) {
    pthread_rwlock_wrlock(&(a->lock));
    if ((index < 0) || (index >= a->in_use)) {
        pthread_rwlock_unlock(&(a->lock));
        return;
    }

    a->dfree(a->data[index]);
    a->data[index] = val;
    pthread_rwlock_unlock(&(a->lock));
}

/***************************************************************************
 * alist_remove take the element at index "index" out of the list
 * (decreasing list size by 1). If the index/position doesn't exist in
 * the list, then nothing happens.
 */
void alist_remove(alist *a, int index) {
    pthread_rwlock_wrlock(&(a->lock));
    if ((index < 0) || (index >= a->in_use)) {
        pthread_rwlock_unlock(&(a->lock));
        return;
    }

    a->dfree(a->data[index]);
    for (int i=index; i<a->in_use-1; i++)
        a->data[i] = a->data[i+1];
    a->in_use--;
    pthread_rwlock_unlock(&(a->lock));
}

/***************************************************************************
 * alist_destroy destroys the current array list, freeing up all memory
 * and resources.
 */
void alist_destroy(alist *a) {
    pthread_rwlock_wrlock(&(a->lock));
    for (int i=0; i<a->in_use; i++) {
        a->dfree(a->data[i]);
    }

    a->in_use = 0;
    free(a->data);
    a->data = NULL;
    a->capacity = 0;
    pthread_rwlock_unlock(&(a->lock));
}