#include "taxiqueue.h"
#include "alist.h"
#include "airplane.h"
#include "planelist.h"
#include "airs_protocol.h"
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

static alist taxi_queue;
static pthread_mutex_t queue_mutex;
static pthread_cond_t queue_cond;
static pthread_t queue_manager_thread;

void *taxiqueue_manager(void *arg);

// Initialize the taxi queue
void taxiqueue_init() {
    alist_init(&taxi_queue, free);
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_cond, NULL);
    pthread_create(&queue_manager_thread, NULL, taxiqueue_manager, NULL);
}

// Add a new flight to the taxi queue
void taxiqueue_add(const char *flight_id) {
    pthread_mutex_lock(&queue_mutex);
    char *id_copy = strdup(flight_id); // Duplicate the ID string
    alist_add(&taxi_queue, id_copy);
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

// Get the position of a flight in the taxi queue
int taxiqueue_getpos(const char *flight_id) {
    pthread_mutex_lock(&queue_mutex);

    int position = 0;
    for (int i = 0; i < alist_size(&taxi_queue); i++) {
        char *current_id = (char *)alist_get(&taxi_queue, i);
        if (strcmp(current_id, flight_id) == 0) {
            position = i + 1; // Position is 1-indexed  
            break;
        }
    }

    pthread_mutex_unlock(&queue_mutex);
    return position;  
}


// Get a string listing all flights ahead of the given flight in the queue
char *taxiqueue_getahead(const char *flight_id) {
    pthread_mutex_lock(&queue_mutex);

    // Determine the position of the flight in the queue
    int pos = 0;
    int size = alist_size(&taxi_queue);
    for (int i = 0; i < size; ++i) {
        char *current_id = alist_get(&taxi_queue, i);
        if (strcmp(current_id, flight_id) == 0) {
            pos = i + 1; 
            break;
        }
    }

    // If the position is invalid or no planes are ahead, return an empty string
    if (pos <= 1 || pos > size) {
        pthread_mutex_unlock(&queue_mutex);
        return strdup("");  // No planes ahead or invalid position
    }

    // Calculate the length required for the response string
    size_t length = 0;
    for (int i = 0; i < pos - 1; ++i) {
        char *current_id = alist_get(&taxi_queue, i);
        length += strlen(current_id) + 2; 
    }

    // Allocate memory for the ahead list string
    char *aheadList = malloc(length);
    if (!aheadList) {
        pthread_mutex_unlock(&queue_mutex);
        return NULL;  // If failure
    }

    // Construct the list of flights ahead
    char *ptr = aheadList;
    for (int i = 0; i < pos - 1; ++i) {
        char *current_id = alist_get(&taxi_queue, i);
        ptr += sprintf(ptr, "%s%s", current_id, (i < pos - 2 ? ", " : ""));
    }

    pthread_mutex_unlock(&queue_mutex);
    return aheadList;
}


// Handle the situation when a plane is in the air
void taxiqueue_inair(const char *flight_id) {
    pthread_mutex_lock(&queue_mutex);

    // Find the airplane in the queue and remove it
    for (int i = 0; i < alist_size(&taxi_queue); i++) {
        char *current_id = (char *)alist_get(&taxi_queue, i);
        if (strcmp(current_id, flight_id) == 0) {
            alist_remove(&taxi_queue, i);     // Remove from the queue
            break;
        }
    }

    pthread_cond_signal(&queue_cond); 
    pthread_mutex_unlock(&queue_mutex);
}


void *taxiqueue_manager(void *arg) {
    while (1) {
        pthread_mutex_lock(&queue_mutex);

        // Wait until there is at least one flight in the queue
        while (alist_size(&taxi_queue) == 0) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }

        // Get the ID of the first flight in the queue
        char *first_flight_id = (char *)alist_get(&taxi_queue, 0);
        airplane *first_plane = planelist_find(first_flight_id);
       
        // Wait for the first plane to signal it's in the air
        while (first_plane != NULL && first_plane->state != PLANE_INAIR) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
            first_plane = planelist_find(first_flight_id); // Recheck the first flight
        }

        
        if (first_plane != NULL && first_plane->state == PLANE_INAIR) {
            alist_remove(&taxi_queue, 0);
            printf("Flight %s has taken off.\n", first_flight_id);
        }

        // Clear the next plane for takeoff if there is one
        if (alist_size(&taxi_queue) > 0) {
            // Sleep interval
            pthread_mutex_unlock(&queue_mutex);
            sleep(4); 
            pthread_mutex_lock(&queue_mutex);
            char *next_flight_id = (char *)alist_get(&taxi_queue, 0);
            airplane *next_plane = planelist_find(next_flight_id);
            if (next_plane != NULL && next_plane->state == PLANE_TAXIING) {
                next_plane->state = PLANE_CLEAR;
                fprintf(next_plane->fp_send, "TAKEOFF\n");
                fflush(next_plane->fp_send); // Ensure the message is sent immediately
                printf("Clearing flight %s for takeoff.\n", next_flight_id);
                pthread_cond_signal(&queue_cond); // Signal that a plane has been cleared
            }
        }

        pthread_mutex_unlock(&queue_mutex);
    }
    return NULL;
}
