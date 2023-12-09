#ifndef _AIRPLANE_H
#define _AIRPLANE_H

#include <stdio.h>
#include <pthread.h>

// The maximum length of a plane id

#define PLANE_MAXID 20

// These are the valid states of an airplane. The numbers don't mean
// anything, and just need to be all different. Note that a more "modern"
// way of doing this would be to use an "enum", but most C programmers
// stick to this old-fashioned way of doing things - so we will too!

#define PLANE_UNREG 0
#define PLANE_DONE 1
#define PLANE_ATTERMINAL 2
#define PLANE_TAXIING 3
#define PLANE_CLEAR 4
#define PLANE_INAIR 5

// The struct to keep track of all information about an airplane in
// the system.

typedef struct airplane {
    int state;
    pthread_t thread;
    FILE *fp_send;
    FILE *fp_recv;
    char id[PLANE_MAXID+1];
} airplane;

// Basic initializer and destructor functions

void airplane_init(airplane *plane, FILE *fp_send, FILE *fp_recv);
airplane *new_airplane(int comm_fd);
void airplane_destroy(airplane *plane);

#endif  // _AIRPLANE_H