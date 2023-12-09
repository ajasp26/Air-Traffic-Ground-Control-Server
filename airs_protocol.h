// Defines the publicly-callable functions in the airs_commands module

#ifndef _AIRS_COMMANDS_H
#define _AIRS_COMMANDS_H


void send_ok(airplane *plane);
void send_err(airplane *plane, char *desc);
void send_err_sarg(airplane *plane, char *fmtstring, char *sarg);

void docommand(airplane *plane, char *command);

#endif  // _AIRS_COMMANDS_H