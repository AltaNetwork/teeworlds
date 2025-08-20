#ifndef MAIN_H
#define MAIN_H

#include <signal.h>
#include "econ.h"

#define MAX_CONNECTIONS 3

// Declare the global variable here so it can be accessed by all source files.
extern EconConn *connections[MAX_CONNECTIONS];

void handle_sigint(int sig);
void tick(EconConn *conn, int conn_id);

#endif
