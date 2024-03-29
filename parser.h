#ifndef PARSER_H
#define PARSER_H

#include "protocol.h"
#include "url.h"

#include <stdlib.h>

/**
 * A parser
 */
struct parser {
  /**
   * A pointer to the data
   */
  const char * data;

  /**
   * The length of the data
   */
  size_t len;

  /**
   * The current position of the cursor
   */
  size_t pos;
};

/**
 * Initializes a parser
 */
void init_parser(struct parser * p);

/**
 * Resets the parser
 */
void clear_parser(struct parser *  p);

/**
 * Loads data into the parser
 */
void load_into_parser(struct parser * p, const char * data, size_t len);

/**
 * Parses a request
 */
int parse_request(struct parser * p, enum http_method * method, struct url_buffer * url);

/**
 * Disposes of a parser
 */
void dispose_parser(struct parser * p);

#endif
