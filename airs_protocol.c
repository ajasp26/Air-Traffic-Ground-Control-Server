// Module to implement the ground controller application-layer protocol.

// The protocol is fully defined in the README file. This module
// includes functions to parse and perform commands sent by an
// airplane (the docommand function), and has functions to send
// responses to ensure proper and consistent formatting of these
// messages.

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


#include "util.h"
#include "airplane.h"
#include "airs_protocol.h"
#include "planelist.h"
#include "taxiqueue.h"

static pthread_mutex_t queue_mutex;

/************************************************************************
 * Call this response function if a command was accepted
 */
void send_ok(airplane *plane) {
    fprintf(plane->fp_send, "OK\n");
}

/************************************************************************
 * Call this response function if an error can be described by a simple
 * string.
 */
void send_err(airplane *plane, char *desc) {
    fprintf(plane->fp_send, "ERR %s\n", desc);
}

/************************************************************************
 * Call this response function if you want to embed a specific string
 * argument (sarg) into an error reply (which is now a format string).
 */
void send_err_sarg(airplane *plane, char *fmtstring, char *sarg) {
    fprintf(plane->fp_send, "ERR ");
    fprintf(plane->fp_send, fmtstring, sarg);
    fprintf(plane->fp_send, "\n");
}

/************************************************************************
 * Handle the "REG" command.
 */
static void cmd_reg(airplane *plane, char *rest) {
    if (plane->state != PLANE_UNREG) {
        send_err_sarg(plane, "Already registered as %s", plane->id);
        return;
    }

    if (rest == NULL) {
        send_err(plane, "REG missing flightid");
        return;
    }

    char *cp = rest;
    while (*cp != '\0') {
        if (!isalnum(*cp)) {
            send_err(plane, "Invalid flight id -- only alphanumeric characters allowed");
            return;
        }
        cp++;
    }

    if (strlen(rest) > PLANE_MAXID) {
        send_err(plane, "Invalid flight id -- too long");
        return;
    }

    // Check for a duplicate flight number
    if (planelist_find(rest) != NULL) {
        send_err(plane, "Duplicate flight id");
        return;
    }

    // Using a "planelist" function to change id for an atomic update
    planelist_changeid(plane, rest);
    plane->state = PLANE_ATTERMINAL;

    send_ok(plane);
}

/************************************************************************
 * Handle the "REQTAXI" command.
 */
static void cmd_reqtaxi(airplane *plane, char *rest) {
    if (plane->state == PLANE_UNREG) {
        send_err(plane, "Unregistered plane -- cannot process request");
        return;
    }

     if (plane->state != PLANE_ATTERMINAL) {
        send_err(plane, "Plane must be at the terminal to request taxi");
        return;
    }

    taxiqueue_add(plane->id);
    plane->state = PLANE_TAXIING;
    send_ok(plane);


    if (taxiqueue_getpos(plane->id) == 1) {
        fprintf(plane->fp_send, "TAKEOFF\n");
        plane->state = PLANE_CLEAR;
        printf("Clearing flight %s\n", plane->id);
    }
}

/************************************************************************
 * Handle the "REQPOS" command.
 */
static void cmd_reqpos(airplane *plane, char *rest) {
    if (plane->state == PLANE_UNREG) {
        send_err(plane, "Unregistered plane -- cannot process request");
        return;
    }

    if (plane->state != PLANE_TAXIING) {
        send_err(plane, "Plane not taxiing -- cannot process request");
        return;
    }

    int pos = taxiqueue_getpos(plane->id);
    if (pos == 0) {
        send_err(plane, "Plane not in taxi queue");
    } else {
        char response[50];
        sprintf(response, "OK %d", pos);
        fprintf(plane->fp_send, "%s\n", response);
    }
}


/************************************************************************
 * Handle the "REQAHEAD" command.
 */
static void cmd_reqahead(airplane *plane, char *rest) {
    if (plane->state != PLANE_TAXIING) {
        send_err(plane, "Plane not taxiing -- cannot process request");
        return;
    }

    char *aheadList = taxiqueue_getahead(plane->id);
    if (!aheadList) {
        send_err(plane, "Server error: unable to retrieve planes ahead");
        return;
    }

    if (strlen(aheadList) > 0) {
        fprintf(plane->fp_send, "OK %s\n", aheadList);
    } else {
        fprintf(plane->fp_send, "OK No planes ahead\n");
    }
    fflush(plane->fp_send); // Flush the stream to ensure the response is sent immediately
    free(aheadList); // Free the memory allocated by taxiqueue_getahead
}



/************************************************************************
 * Handle the "INAIR" command.
 */
static void cmd_inair(airplane *plane, char *rest) {
    if (plane->state != PLANE_CLEAR) {
        send_err(plane, "Plane not cleared for takeoff -- cannot process INAIR command");
        return;
    }

    plane->state = PLANE_INAIR;
    taxiqueue_inair(plane->id);  // Remove the plane from the taxi queue

    fprintf(plane->fp_send, "OK\n");
    fprintf(plane->fp_send, "NOTICE Disconnecting from ground control - please connect to air control\n");

    printf("Client %ld disconnected.\n", plane->thread);
    printf("Flight %s is in the air - - waiting 4 seconds\n", plane->id);
    plane->state = PLANE_DONE;

}



/************************************************************************
 * Handle the "BYE" command.
 */
static void cmd_bye(airplane *plane, char *rest) {
    plane->state = PLANE_DONE;
}

/************************************************************************
 * Parses and performs the actions in the line of text (command and
 * optionally arguments) passed in as "command".
 */
void docommand(airplane *plane, char *command) {
    char *saveptr;
    char *cmd = strtok_r(command, " \t\r\n", &saveptr);
    if (cmd == NULL) {  // Empty line (no command) -- just ignore line
        return;
    }

    // Get arguments (everything after command, trimmed)
    char *args = strtok_r(NULL, "\r\n", &saveptr);
    if (args != NULL) {
        args = trim(args);
    }

    if (strcmp(cmd, "REG") == 0) {
        cmd_reg(plane, args);
    } else if (strcmp(cmd, "REQTAXI") == 0) {
        cmd_reqtaxi(plane, args);
    } else if (strcmp(cmd, "REQPOS") == 0) {
        cmd_reqpos(plane, args);
    } else if (strcmp(cmd, "REQAHEAD") == 0) {
        cmd_reqahead(plane, args);
    } else if (strcmp(cmd, "INAIR") == 0) {
        cmd_inair(plane, args);
    } else if (strcmp(cmd, "BYE") == 0) {
        cmd_bye(plane, args);
    } else {
        send_err(plane, "Unknown command");
    }
}