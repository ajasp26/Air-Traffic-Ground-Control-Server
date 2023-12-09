// This is the main program for the air traffic ground control server.

// The job of this module is to set the system up and then turn each
// command received from the client over to the airs_protocol module
// which will handle the actual communication protocol between clients
// (airplanes) and the server.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "airplane.h"
#include "airs_protocol.h"
#include "planelist.h"
#include "taxiqueue.h"

/***********************************************************************
 * The client thread handles the basic network read loop -- get a line
 * from the network connection and process it.
 */
static void *client_thread(void *arg) {
    airplane *myplane = (airplane *)arg;
    planelist_add(myplane);

    // Detatch - unpredictable thread start/stop patterns

    pthread_detach(myplane->thread);

    char *lineptr = NULL;
    size_t linesize = 0;

    while (myplane->state != PLANE_DONE) {
        if (getline(&lineptr, &linesize, myplane->fp_recv) < 0) {
            // Failed getline means the client disconnected
            break;
        }
        docommand(myplane, lineptr);
    }

    // Finished with session, so unregister it and free resources.

    if (lineptr != NULL) {
        free(lineptr);
    }

    //printf("Client %ld disconnected.\n", myplane->thread);
    planelist_remove(myplane);

    return NULL;
}

/********************************************************************
 * Make a TCP listener for port "service" (given as a sting, but
 * either a port number or service name). This function will only
 * create a public listener (listening on all interfaces).
 *
 * Either returns a file handle to use with accept(), or -1 on error.
 * In general, error reporting could be improved, but this just indicates
 * success or failure.
 */
static int create_listener(char *service) {
    int sock_fd;
    if ((sock_fd=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    // Avoid time delay in reusing port - important for debugging, but
    // probably not used in a production server.

    int optval = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    // First, use getaddrinfo() to fill in address struct for later bind

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    struct addrinfo *result;
    int rval;
    if ((rval=getaddrinfo(NULL, service, &hints, &result)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rval));
        close(sock_fd);
        return -1;
    }

    // Assign a name/addr to the socket - just blindly grabs first result
    // off linked list, but really should be exactly one struct returned.

    int bret = bind(sock_fd, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);
    result = NULL;  // Not really necessary, but ensures no use-after-free

    if (bret < 0) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    // Finally, set up listener connection queue
    int lret = listen(sock_fd, 128);
    if (lret < 0) {
        perror("listen");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

/************************************************************************
 * Part 2 main: networked server. Spawns a new thread for each connection.
 * Probably should put an upper limit on this, but we're not going to
 * go crazy with a million clients in this class assignment...
 */
int main(int argc, char *argv[]) {
    planelist_init();
    taxiqueue_init();

    int sock_fd = create_listener("8080");
    if (sock_fd < 0) {
        fprintf(stderr, "Server setup failed.\n");
        exit(1);
    }

    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int comm_fd;
    while ((comm_fd=accept(sock_fd, (struct sockaddr *)&client_addr,
                           &client_addr_len)) >= 0) {
        // Got a new connection, so create an "airplane" and spawn a thread
        airplane *new_client = new_airplane(comm_fd);
        if (new_client != NULL) {
            pthread_create(&new_client->thread, NULL, client_thread, new_client);
            printf("Got connection from %s (client %ld)\n",
                   inet_ntoa(((struct sockaddr_in *)&client_addr)->sin_addr),
                   new_client->thread);
        }
    }

    // // Can't actually get here (part 2) - will fix this in part 3!
    // printf("Shutting down...\n");

    return 0;
}