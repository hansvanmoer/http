#include "connection.h"
#include "logger.h"
#include "protocol.h"
#include "server.h"
#include "task.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * The listener socket
 */
static int listen_socket = -1;

/**
 * The task service
 */
static struct task_service task_service;

/**
 * Binds a socket to the specified port
 */
static int bind_socket(int fd, int16_t port) {
  // buffer is too large to suppress warning for sprintf
  char port_buffer[16];

  if(sprintf(port_buffer, "%d", (int)port) < 0) {
    LOG_ERRNO("could not format port number");
    return -1;
  }
  
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  struct addrinfo * address;
  int result;
  if((result = getaddrinfo(NULL, port_buffer, &hints, &address))) {
    LOG_ERROR_CODE("could not get address info", result);
    return -1;
  }
  struct addrinfo * a;
  for(a = address; a != NULL; a = a->ai_next) {
    if(bind(fd, a->ai_addr, a->ai_addrlen) == 0){
      break;
    }
  }
  if(a == NULL) {
    LOG_ERROR("could not bind socket");
    freeaddrinfo(address);
    return -1;
  }
  LOG_INFO("listening at port %s", port_buffer);
  freeaddrinfo(address);
  return 0;
}

/**
 * Serves a client request
 */
static void serve_client(struct connection * c) {
  handle_request(c);
}

/**
 * Runs the task
 */
static void run_client_task(void * data) {
  assert(data != NULL);

  struct connection * c = (struct connection *)data;
  serve_client(c);
}

static void cleanup_client_task(void * data) {
  assert(data != NULL);

  struct connection * c = (struct connection *)data;
  close_connection(c);
}

int start_server(int16_t port, size_t max_connections) {
  LOG_INFO("starting server...");
  if(init_task_service(&task_service,max_connections)) {
    return -1;
  }
  if(init_connections(max_connections)) {
    dispose_task_service(&task_service);
    return -1;
  }
  
  listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(listen_socket == -1) {
    LOG_ERRNO("could not create listen socket");
    dispose_task_service(&task_service);
    dispose_connections();
    return -1;
  }

  if(bind_socket(listen_socket, port)) {
    close(listen_socket);
    dispose_task_service(&task_service);
    dispose_connections();
    listen_socket = -1;
    return -1;
  }

  if(listen(listen_socket, max_connections)) {
    LOG_ERRNO("could not listen on listen socket");
    close(listen_socket);
    dispose_connections();
    dispose_task_service(&task_service);
    listen_socket = -1;
    return -1;
  }
  
  LOG_INFO("server started");
  return 0;
}

int run_server() {
  LOG_DEBUG("accepting connections");

  struct sockaddr client_address;
  socklen_t client_address_len;
  while(true) {
    int result = accept(listen_socket, &client_address, &client_address_len);
    if(result < 0) {
      switch(errno) {
      case EFAULT:
      case EINVAL:
      case EMFILE:
      case ENFILE:
      case ENOBUFS:
      case ENOMEM:
      case ENOTSOCK:
      case EOPNOTSUPP:
      case EPERM:
	LOG_ERRNO("error while listening to socket");
	return -1;
      default:
	break;
      }
    } else {
      struct connection * c = open_connection(result);
      if(c == NULL) {
	// connection refused
	close(result);
      } else {
	if(add_task(&task_service, run_client_task, c, cleanup_client_task)) {
	  LOG_ERROR("could not add client task");
	  close(result);
	  return -1;
	}
      }
    }
    
  }
}

void stop_server() {
  LOG_INFO("stopping server...");
  close(listen_socket);
  dispose_task_service(&task_service);
  dispose_connections();
  LOG_INFO("server stopped");
}
