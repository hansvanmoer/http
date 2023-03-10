#ifndef URL_H
#define URL_H

#include <stdlib.h>
#include <stdint.h>

/**
 * An URL buffer
 */
struct url_buffer {
  
  /**
   * The host
   */
  char * host;

  /**
   * The capacity of the host string buffer
   */
  size_t host_cap;

  /**
   * The port
   */
  uint16_t port;
  
  /**
   * The path
   */
  char * path;

  /**
   * The capacity of the path string buffer
   */
  size_t path_cap;
};

/**
 * Initializes an URL buffer
 */
void init_url_buffer(struct url_buffer * url);

/**
 * Disposes of an URL buffer
 */
void dispose_url_buffer(struct url_buffer * url);

#endif
