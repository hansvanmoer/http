#include "buffer.h"
#include "connection.h"
#include "logger.h"
#include "parser.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * The connection buffer
 */
static struct connection * connections;

/**
 * The max number of connections
 */
static size_t max_amount;

/**
 * The number of connections in use
 */
static size_t active_amount;

/**
 * The mutex protecting the connection data structures
 */
static pthread_mutex_t mutex;

/**
 * Initializes a connection
 */
static void init_connection(struct connection * c) {
  assert(c != NULL);

  c->socket = -1;
  init_text_buffer(&c->buffer);
}

/**
 * Disposes of a connection
 */
static void dispose_connection(struct connection * c) {
  assert(c != NULL);

  dispose_text_buffer(&c->buffer);
  if(c->socket != -1) {
    close(c->socket);
  }
}

/**
 * Initializes the connection buffer
 */
int init_connections(size_t _max_amount) {
  if(max_amount == 0) {
    LOG_ERROR("max amount of connections can not be 0");
    return -1;
  }
  max_amount = _max_amount;
  int result;
  if((result = pthread_mutex_init(&mutex, NULL))) {
    LOG_ERROR_CODE("could not create connection mutex", result);
    return -1;
  }

  connections = (struct connection *) malloc(sizeof(struct connection) * max_amount);
  if(connections == NULL){
    LOG_ERRNO("could not allocate connection buffer");
    pthread_mutex_destroy(&mutex);
    return -1;
  }
  for(size_t i = 0; i < max_amount; ++i) {
    init_connection(connections + i);
  }
  active_amount = 0;
  return 0;
}

/**
 * Opens a new connection
 */
struct connection * open_connection(int socket) {
  int result;
  if((result = pthread_mutex_lock(&mutex))) {
    LOG_ERROR_CODE("could not lock connection mutex", result);
    return NULL;
  }
  if(active_amount == max_amount) {
    return NULL;
  }
  size_t index;
  for(index = 0; index < max_amount; ++index) {
    if(connections[index].socket == -1) {
      break;
    }
  }
  assert(index != max_amount);
  ++active_amount;
  connections[index].socket = socket;
  if((result = pthread_mutex_unlock(&mutex))) {
    LOG_ERROR_CODE("could not unlock connection mutex", result);
    connections[index].socket = -1;
    return NULL;
  }
  return connections + index;
}

/**
 * Closes a connection
 */
void close_connection(struct connection * c) {
  if(c->socket != -1) {
    close(c->socket);
  }
  int result;
  if((result = pthread_mutex_lock(&mutex))) {
    LOG_ERROR_CODE("could not lock connection mutex", result);
  }
  assert(active_amount != 0);
  --active_amount;
  c->socket = -1;
  if((result = pthread_mutex_unlock(&mutex))) {
    LOG_ERROR_CODE("could not unlock connection mutex", result);
  }
}

/**
 * Disposes of the connection buffer
 */
void dispose_connections() {
  int result;
  if((result = pthread_mutex_lock(&mutex))) {
    LOG_ERROR_CODE("could not lock connection mutex", result);
    return;
  }
  for(size_t i = 0; i < max_amount; ++i) {
    dispose_connection(connections + i);
  }
  if((result = pthread_mutex_unlock(&mutex))) {
    LOG_ERROR_CODE("could not unlock connection mutex", result);
    return;
  }
  if((result = pthread_mutex_destroy(&mutex))) {
    LOG_ERROR_CODE("could not destroy connection mutex", result);
  }
  free(connections);
}
