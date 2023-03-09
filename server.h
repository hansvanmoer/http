#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>

/**
 * Starts the server
 */
int start_server(int16_t port, size_t max_connections);

/**
 * Stops the server
 */
void stop_server();

#endif
