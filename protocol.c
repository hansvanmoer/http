#include "buffer.h"
#include "parser.h"
#include "protocol.h"

#include <errno.h>

/**
 * Maximum request size
 */
#define PROTOCOL_MAX_REQUEST_LEN 1024

/**
 * Bad request
 */
#define PROTOCOL_BAD_REQUEST 400

/**
 * Line delimiter
 */
#define PROTOCOL_LINE_DELIMITER "\r\n"

/**
 * Reject the request
 */
static int reject(struct connection * c, enum http_status_code status_code) {
  return -1;
}

int handle_request(struct connection *c){
  size_t remainder = PROTOCOL_MAX_REQUEST_LEN;
  
  if(read_until_string(&c->buffer, c->socket, PROTOCOL_LINE_DELIMITER, remainder)) {
    switch(errno) {
    case E2BIG:
      return reject(c, HTTP_STATUS_CODE_BAD_REQUEST);
    default:
      return reject(c, HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR);
    }
  }
  struct parser parser;
  init_parser(&parser);

  load_into_parser(&parser, c->buffer.data, c->buffer.len);

  if(1) {
    dispose_parser(&parser);
    return reject(c, HTTP_STATUS_CODE_BAD_REQUEST);
  }

  dispose_parser(&parser);

  return 0;
}
