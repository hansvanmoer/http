#indef URL_H
#define URL_H

#include <stdlib.h>

/**
 * An URL buffer
 */
struct url_buffer {
  
  /**
   * The host
   */
  const char * host;

  /**
   * The capacity of the host string buffer
   */
  size_t host_cap;

  /**
   * The path
   */
  const char * path;

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
