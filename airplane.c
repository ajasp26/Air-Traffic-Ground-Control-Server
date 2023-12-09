// The airplane module contains the airplane data type and management functions

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "airplane.h"

/************************************************************************
 * plane_init initializes an airplane structure in the initial PLANE_UNREG
 * state, with given send and receive FILE objects.
 */
void airplane_init(airplane *plane, FILE *fp_send, FILE *fp_recv) {
    plane->state = PLANE_UNREG;
    plane->fp_send = fp_send;
    plane->fp_recv = fp_recv;
    plane->id[0] = '\0';
}

/************************************************************************
 * new_airplane allocates an airplane struct and initializes it for
 * file descriptor "comm_fd". If any of the setup fails, this returns
 * NULL (should never happen?).
 */
airplane *new_airplane(int comm_fd) {
    airplane *ret = malloc(sizeof(airplane));
    if (ret == NULL) {
        perror("new_airplane");
        exit(1);
    }

    // Duplicate the file descriptor so we have separare read and write fds
    // There may be a better way to do this, but using the same fd for both
    // read and write seems to confuse the position in I/O buffering.
    int dup_fd = dup(comm_fd);
    if (dup_fd < 0) {
        perror("new_airplane dup");
        free(ret);
        return NULL;
    }

    // Wrap the original fd in a FILE* for buffered/formatted writing
    FILE *sender = fdopen(comm_fd, "w");
    if (sender == NULL) {
        perror("new_airplane fd_open sender");
        close(dup_fd);
        close(comm_fd);
        free(ret);
        return NULL;
    }

    // Wrap the duplicate fd in a FILE* for buffered/formatted reading
    FILE *receiver = fdopen(dup_fd, "r");
    if (receiver == NULL) {
        perror("new_airplane fd_open receiver");
        fclose(sender);
        close(dup_fd);
        free(ret);
        return NULL;
    }

    // Set both FILE*'s to be line buffered (line-oriented app protocol)

    setvbuf(sender, NULL, _IOLBF, 0);
    setvbuf(receiver, NULL, _IOLBF, 0);

    airplane_init(ret, sender, receiver);
    return ret;
}

/************************************************************************
 * plane_destroy frees up any resources associated with an airplane, like
 * file handles, so that it can be free'ed. Will be called from a planelist
 * function to ensure that it is out of the list (and won't be accessed
 * again) before it is destroyed.
 */
void airplane_destroy(airplane *plane) {
    plane->state = PLANE_DONE;  // Just to make sure....
    fclose(plane->fp_send);
    fclose(plane->fp_recv);
}