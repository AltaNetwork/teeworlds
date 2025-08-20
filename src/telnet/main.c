#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#include "main.h" // Include the new header file
#include "econ.h"

// Define the global connections array here.
EconConn *connections[MAX_CONNECTIONS];

typedef struct {
    int Port;
    char *Adress;
    char *Password;
} EconServers;

EconServers Servers[MAX_CONNECTIONS] = {
    {8304, "127.0.0.1", "milebaba"},
    {8305, "127.0.0.1", "milebaba"},
    {8306, "127.0.0.1", "milebaba"}
};

int TimeOut[MAX_CONNECTIONS];
volatile sig_atomic_t keep_running = 1;
time_t last_reconnect_attempt[MAX_CONNECTIONS];
const int RECONNECT_INTERVAL = 5;

// The tick function now has access to the global `connections` array
void tick(EconConn *conn, int conn_id) {
    char *line;
    if ((line = econ_read_line(conn)) != NULL) {
        printf("%s", line);
        int id;
        char username[50];
        char password[50];
        int parsed_items = sscanf(line, "[login]: %d;%49[^;];%49s", &id, username, password);

        if (parsed_items == 3) {
            // printf("ID: %d\n", id);
            // printf("Username: %s\n", username);
            // printf("Password: %s\n", password);
            char aBuf[128];
            snprintf(aBuf, sizeof(aBuf), "broadcast id:%d\nuser:%s\npwd:%s", id, username, password);
            econ_write_line(conn, aBuf);
        } else {
            printf("Failed to parse the string. Parsed items: %d\n", parsed_items);
        }
        // econ_write_line(conn, "");
        free(line);
    } else {
        // Corrected: Set the global connections array pointer to NULL
        connections[conn_id] = NULL;
    }
}

void initconn(int conn_id) {
    TimeOut[conn_id] = 5000;
}

void handle_sigint(int sig) {
    printf("\nSIGINT Cleanup\n");
    keep_running = 0;
}

int main() {
    signal(SIGINT, handle_sigint);
    int i;

    for (i = 0; i < MAX_CONNECTIONS; i++) {
        connections[i] = NULL;
        last_reconnect_attempt[i] = 0;
    }

    while (keep_running) {
        for (i = 0; i < MAX_CONNECTIONS; i++) {
            if (connections[i] == NULL) {
                if (time(NULL) - last_reconnect_attempt[i] >= RECONNECT_INTERVAL) {
                    printf("Trying to reconnect to %s:%d... ", Servers[i].Adress, Servers[i].Port);
                    connections[i] = econ_dial_to(Servers[i].Adress, Servers[i].Port, Servers[i].Password);

                    if (connections[i]) {
                        printf("success\n");
                        initconn(i);
                    } else {
                        printf("fail\n");
                    }
                    last_reconnect_attempt[i] = time(NULL);
                }
            } else {
                tick(connections[i], i);
            }
        }
    }

    printf("Closing connections...\n");
    for (i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i]) {
            econ_close(connections[i]);
        }
    }

    return 0;
}
