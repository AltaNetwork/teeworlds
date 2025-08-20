#ifndef ECON_H
#define ECON_H

#include <stdio.h>
#include <stdbool.h>
#include <libtelnet.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Represents an ECON connection
typedef struct {
    int sockfd;
    struct sockaddr_in server_addr;
    telnet_t *telnet;
    char *password;
} EconConn;

// Function to initialize a new ECON connection
EconConn* econ_dial_to(const char* address, int port, const char* password);

// Function to send a command to the ECON console
int econ_write_line(EconConn *conn, const char* line);

// Function to read a line from the ECON console
char* econ_read_line(EconConn *conn);

// Function to close the connection and free resources
void econ_close(EconConn *conn);

#endif
