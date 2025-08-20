#include "econ.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// A simple state machine handler for libtelnet events
static void telnet_event_handler(telnet_t *telnet, telnet_event_t *event, void *user_data) {
    // This handler can be used to manage Telnet-specific events, but for a simple ECON client,
    // we often only care about the data events.
    (void)telnet;
    (void)user_data;
    switch (event->type) {
        case TELNET_EV_DATA:
            // Data received from the server
            // Can be used to read and buffer data
            break;
        default:
            break;
    }
}

EconConn* econ_dial_to(const char* address, int port, const char* password) {
    EconConn *conn = malloc(sizeof(EconConn));
    if (!conn) {
        return NULL;
    }
    memset(conn, 0, sizeof(EconConn));

    conn->password = strdup(password);
    if (!conn->password) {
        free(conn);
        return NULL;
    }

    // Create a socket
    conn->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn->sockfd < 0) {
        free(conn->password);
        free(conn);
        return NULL;
    }

    conn->server_addr.sin_family = AF_INET;
    conn->server_addr.sin_port = htons(port);
    inet_pton(AF_INET, address, &conn->server_addr.sin_addr);

    // Connect to the server
    if (connect(conn->sockfd, (struct sockaddr*)&conn->server_addr, sizeof(conn->server_addr)) < 0) {
        close(conn->sockfd);
        free(conn->password);
        free(conn);
        return NULL;
    }

    // Correctly initialize libtelnet with the required arguments
    // The first argument `telopts` is for Telnet options negotiation, which isn't
    // needed for a simple Teeworlds RCON client. Pass NULL.
    conn->telnet = telnet_init(NULL, telnet_event_handler, 0, NULL);
    if (!conn->telnet) {
        close(conn->sockfd);
        free(conn->password);
        free(conn);
        return NULL;
    }

    // Simple authentication
    char buffer[256];
    ssize_t n_read;

    // Read the "Enter password:" prompt
    n_read = recv(conn->sockfd, buffer, sizeof(buffer) - 1, 0);
    if (n_read <= 0) {
        econ_close(conn);
        return NULL;
    }
    buffer[n_read] = '\0';
    if (strstr(buffer, "Enter password:") == NULL) {
        econ_close(conn);
        return NULL;
    }

    // Send the password
    char password_with_newline[256];
    snprintf(password_with_newline, sizeof(password_with_newline), "%s\n", conn->password);
    send(conn->sockfd, password_with_newline, strlen(password_with_newline), 0);

    // Read the "Authentication successful" message
    n_read = recv(conn->sockfd, buffer, sizeof(buffer) - 1, 0);
    if (n_read <= 0) {
        econ_close(conn);
        return NULL;
    }
    buffer[n_read] = '\0';
    if (strstr(buffer, "Authentication successful") == NULL) {
        econ_close(conn);
        return NULL;
    }

    return conn;
}

int econ_write_line(EconConn *conn, const char* line) {
    char full_command[1024];
    snprintf(full_command, sizeof(full_command), "%s\n", line);
    return send(conn->sockfd, full_command, strlen(full_command), 0);
}

char* econ_read_line(EconConn *conn) {
    char buffer[1024];
    ssize_t n_read = recv(conn->sockfd, buffer, sizeof(buffer) - 1, 0);
    if (n_read <= 0) {
        return NULL; // Connection closed or error
    }
    buffer[n_read] = '\0';
    return strdup(buffer); // Remember to free this string after use
}

void econ_close(EconConn *conn) {
    if (conn) {
        if (conn->telnet) {
            telnet_free(conn->telnet);
        }
        if (conn->sockfd >= 0) {
            close(conn->sockfd);
        }
        free(conn->password);
        free(conn);
    }
}
