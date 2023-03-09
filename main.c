#include <stdio.h>
#include <stdlib.h>

#include "logger.h"
#include "server.h"

/**
 * Main application entry point
 */
int main(int arg_count, const char * args[]) {
  init_logger(stdout, LOG_PRIORITY_DEBUG);

  if(start_server(8090, 10) == 0) {
    stop_server();
  }
  
  dispose_logger();
  return EXIT_SUCCESS;
}
