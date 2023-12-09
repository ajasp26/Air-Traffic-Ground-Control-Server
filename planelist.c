// This module is built on top of alist.h, to provide for a threadsafe
// list of airplanes in the system. There is one global list, managed
// by this module, and it is locked any time the list is accessed or
// changes in some way.

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "alist.h"
#include "planelist.h"

// The array list of all planes

static alist all_planes;

// A global lock, to ensure that the list doesn't change when being accessed

static pthread_rwlock_t listlock;

/***************************************************************************
 * Callback function for use by the alist routines to destroy and free an
 * airplane struct.
 */
void airplane_free(void *p) {
    airplane *ap = (airplane *)p;
    airplane_destroy(ap);
    free(ap);
}

/***************************************************************************
 * Initializes the list of planes. Should be called once at the beginning
 * of main, when the program starts up.
 */
void planelist_init(void) {
    alist_init(&all_planes, airplane_free);
    pthread_rwlock_init(&listlock, NULL);
}

/***************************************************************************
 * planelist_add adds a new airplane entry to the list.
 */
void planelist_add(airplane *newplane) {
    pthread_rwlock_wrlock(&listlock);
    alist_add(&all_planes, newplane);
    pthread_rwlock_unlock(&listlock);
}

/***************************************************************************
 * planelist_changeid doesn't change the structure of the list at all,
 * but is provided so that the plane's id can be updated atomically. If
 * the id were mid-change when the list was scanned (e.g., by planelist_find)
 * there would be problems.
 */
void planelist_changeid(airplane *plane, char *newid) {
    pthread_rwlock_wrlock(&listlock);
    strcpy(plane->id, newid);
    pthread_rwlock_unlock(&listlock);
}

/***************************************************************************
 * planelist_find searches the list of planes for a registered airplane with
 * the given flightid. Returns either that airplane struct or NULL if no
 * such airplane is in the list.
 */
airplane *planelist_find(char *flightid) {
    pthread_rwlock_rdlock(&listlock);
    for (int i=0; i<alist_size(&all_planes); i++) {
        airplane *thisplane = alist_get(&all_planes, i);
        if ((thisplane->state != PLANE_UNREG) &&
            (strcmp(thisplane->id, flightid) == 0) ) {
            pthread_rwlock_unlock(&listlock);
            return thisplane;
        }
    }

    pthread_rwlock_unlock(&listlock);
    return NULL;
}

/***************************************************************************
 * planelist_remove scans the list of airplanes for the specific struct
 * passed in, and then removes it from the list. Typically this is called
 * from a thread for their own airplane before the thread exits.
 */
void planelist_remove(airplane *ditch) {
    pthread_rwlock_wrlock(&listlock);
    for (int i=0; i<alist_size(&all_planes); i++) {
        if (alist_get(&all_planes, i) == ditch) {
            alist_remove(&all_planes, i);
            pthread_rwlock_unlock(&listlock);
            return;
        }
    }

    printf("Couldn't find plane to remove - this shouldn't happen\n");
    pthread_rwlock_unlock(&listlock);
}