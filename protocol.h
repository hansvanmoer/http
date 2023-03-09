#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "connection.h"

/**
 * All supported HTTP status codes
 */
enum http_status_code {
  /**
   * OK
   */
  HTTP_STATUS_CODE_OK = 200,

  /**
   * Bad request
   */
  HTTP_STATUS_CODE_BAD_REQUEST = 400,

  /**
   * Internal server error
   */
  HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR = 500
};

/**
 * Supported HTTP methods
 */
enum http_method {
  /**
   * GET
   */
  HTTP_METHOD_GET
};

/**
 * Handles a request
 */
int handle_request(struct connection * c);

#endif
