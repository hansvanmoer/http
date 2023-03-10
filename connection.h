#ifndef CONNECTION_H
#define CONNECTION_H

#include "buffer.h"

#include <stdlib.h>

/**
 * All state associated with a connection
 */
struct connection {
  /**
   * The local socket
   */
  int socket;
  
  /**
   * A text buffer
   */
  struct text_buffer buffer;
};

/**
 * Initializes the connection buffer
 */
int init_connections(size_t max_amount);

/**
 * Opens a new connection
 */
struct connection * open_connection(int socket);

/**
 * Closes a connection
 */
void close_connection(struct connection * c);

/**
 * Disposes of the connection buffer
 */
void dispose_connections();

#endif
