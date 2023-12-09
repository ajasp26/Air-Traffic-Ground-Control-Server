// Module for "utility functions" - general functions that provide useful
// functionality for the ground control server, but are not tied to a
// particular data type or module.

#include <string.h>
#include <ctype.h>

#include "util.h"

/************************************************************************
 * "trim" is like the Java "trim" String method (or the "strip"
 * function in Python): It removes all whitespace at the beginning and
 * at the end of a string. It does this in place, by writing the NUL
 * byte at the appropriate ending position, and returning a pointer to
 * the beginning of the string with whitespace removed. Since it
 * modifies the string, it must be called with a writable buffer
 * (i.e., not with a literal).
 */
char *trim(char *line) {
    int llen = strlen(line);
    if (llen > 0) {
        char *final = &line[llen-1];
        while ((final >= line) && isspace(*final))
            final--;
        *(final+1) = '\0';
    }

    while (isspace(*line))
        line++;

    return line;
}