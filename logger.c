#define __POSIX_C_SOURCE 200809L

#include "logger.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>


/**
 * Buffer for errno messages
 */
#define ERROR_CODE_BUFFER_SIZE 128

/**
 * A log message
 */
struct log_msg {
  /**
   * The message buffer
   */
  char * buffer;
  /**
   * The message buffer capacity
   */
  size_t cap;
  /**
   * The length of the current message
   */
  size_t len;
  /**
   * The file where the message originated
   */
  const char * file;
  /**
   * The line where the message originated
   */
  int line;
  /**
   * The priority
   */
  enum log_priority priority;
  /**
   * A pointer to the next message in the queue
   */
  struct log_msg * next;
};

/**
 * A queue of log messages
 */
struct log_queue {
  /**
   * A pointer to the first message
   */
  struct log_msg * head;

  /**
   * A pointer to the last message
   */
  struct log_msg * tail;
};

/**
 * The queue of messages waiting to be processed
 */
static struct log_queue waiting;

/**
 * The mutex protecting the waiting queue
 */
static pthread_mutex_t waiting_mutex;

/**
 * The condition variable to signal that messages are waiting to be processed
 */
static pthread_cond_t waiting_cond;

/**
 * The queue of messages ready to be reused
 */
static struct log_queue ready;

/**
 * The mutex protecting the ready queue
 */
static pthread_mutex_t ready_mutex;

/**
 * The worker thread
 */
static pthread_t worker;

/**
 * Whether the system is running or not
 */
static bool running = false;

/**
 * The output file
 */
static FILE * output;

/**
 * The minimum priority
 */
static enum log_priority min_priority = LOG_PRIORITY_ERROR;

/**
 * Log message labels
 */
static const char * priority_labels[] = {
  "DEBUG:  ",
  "INFO:   ",
  "WARNING:",
  "ERROR:  "
};

/**
 * Creates a log message and returns it (or NULL on error)
 */
static struct log_msg * create_log_msg() {
  struct log_msg * msg = malloc(sizeof(struct log_msg));
  if(msg == NULL) {
    return NULL;
  }
  msg->buffer = NULL;
  msg->cap = 0;
  msg->len = 0;
  msg->priority = LOG_PRIORITY_DEBUG;
  msg->line = 0;
  msg->file = NULL;
  msg->next = NULL;
  return msg;
}

/**
 * Grows the buffer of this log message if its capacity is smaller
 * than the specified value
 */
static int ensure_log_msg_buffer(struct log_msg * msg, size_t cap) {
  assert(msg != NULL);
  if(cap > msg->cap) {
    char * buffer = malloc(cap);
    if(buffer == NULL) {
      return -1;
    }
    free(msg->buffer);
    msg->buffer = buffer;
    msg->cap = cap;
    msg->len = 0;
  }
  return 0;
}

/**
 * Destroys the supplied message, including its buffer
 */
static void destroy_log_msg(struct log_msg * msg) {
  assert(msg != NULL);
  if(msg->buffer != NULL) {
    free(msg->buffer);
  }
  free(msg);
}

/**
 * Inititializes a log queue
 */
static void init_log_queue(struct log_queue * q) {
  assert(q != NULL);
  q->head = NULL;
  q->tail = NULL;
}

/**
 * Pops a message from a log queue
 */
static struct log_msg * pop_from_log_queue(struct log_queue * q) {
  assert(q != NULL);
  if(q->head == NULL) {
    return NULL;
  } else {
    struct log_msg * m = q->head;
    q->head = m->next;
    if(q->head == NULL) {
      q->tail = NULL;
    }
    return m;
  }
}

/**
 * Push a message onto a log queue
 */
static void push_onto_log_queue(struct log_queue * q, struct log_msg * msg) {
  assert(q != NULL);
  assert(msg != NULL);
  if(q->tail == NULL) {
    assert(q->head == NULL);
    q->head = msg;
    q->tail = msg;
  } else {
    q->tail->next = msg;
    q->tail = msg;
  }
  msg->next = NULL;
}

/**
 * Shift all messages from the source to the target queue
 */
static void shift_between_log_queues(struct log_queue * dest, struct log_queue * src) {
  assert(dest != NULL);
  assert(src != NULL);
  if(src->head != NULL){
    if(dest->tail == NULL) {
      assert(dest->head == NULL);
      dest->head = src->head;
      dest->tail = src->tail;
    } else {
      assert(dest->head != NULL);
      dest->tail->next = src->head;
      dest->tail = src->tail;
    }
    src->head = NULL;
    src->tail = NULL;
  }
}

/**
 * Whether the queue is empty
 */
static bool log_queue_is_empty(const struct log_queue * q) {
  assert(q != NULL);
  return q->head == NULL;
}

/**
 * Disposes a log queue, destorying all messages still on it
 */
static void dispose_log_queue(struct log_queue * q) {
  assert(q != NULL);
  struct log_msg * m = q->head;
  while(m != NULL) {
    struct log_msg * next = m->next;
    destroy_log_msg(m);
    m = next;
  }
}

/**
 * Returns a log message that is ready for use, or NULL if none were availabel
 * and creating one failed
 */
static struct log_msg * get_ready_log_msg() {
  if(pthread_mutex_lock(&ready_mutex)) {
    return NULL;
  }
  struct log_msg * msg = pop_from_log_queue(&ready);
  if(pthread_mutex_unlock(&ready_mutex)){
    if(msg != NULL) {
      destroy_log_msg(msg);
    }
    return NULL;
  }
  if(msg == NULL) {
    msg = create_log_msg();
  }
  return msg;
}

/**
 * Recycle log messages
 */
static int recycle_log_messages(struct log_queue * q) {
  assert(q != NULL);
  if(!log_queue_is_empty(q)){
    if(pthread_mutex_lock(&ready_mutex)) {
      return -1;
    }
    shift_between_log_queues(&ready, q);
    if(pthread_mutex_unlock(&ready_mutex)) {
      return -1;
    }
  }
  return 0;
}

/**
 * Prints a log message
 */
static int print_log_msg(const struct log_msg * msg) {
  assert(msg != NULL);
  if(fprintf(output, "%s%s:%d:%s\n", priority_labels[msg->priority], msg->file, msg->line, msg->buffer) < 0) {
    return -1;
  }
  return 0;
}

/**
 * Worker thread logging function
 */
static void * log_messages(void *) {
  struct log_queue active;
  init_log_queue(&active);
  bool loop = true;
  
  while(loop) {
    if(pthread_mutex_lock(&waiting_mutex)) {
      break;
    }
    if(!running){
      loop = false;
    }else if(log_queue_is_empty(&waiting)){
      if(pthread_cond_wait(&waiting_cond, &waiting_mutex)) {
	pthread_mutex_unlock(&waiting_mutex);
	break;
      }
    }
    if(!log_queue_is_empty(&waiting)){
      shift_between_log_queues(&active, &waiting);
    }
    if(pthread_mutex_unlock(&waiting_mutex)){
      break;
    }
    
    struct log_msg * msg = active.head;
    while(msg != NULL) {
      if(print_log_msg(msg)){
	break;
      }
      msg = msg->next;
    }
    if(recycle_log_messages(&active)){
      break;
    }
  }
  dispose_log_queue(&active);
  return NULL;
}

/**
 * Starts a logger
 */
int init_logger(FILE * _output, enum log_priority _min_priority) {
  assert(_output != NULL);
  assert(_min_priority == LOG_PRIORITY_DEBUG ||
	 _min_priority == LOG_PRIORITY_INFO ||
	 _min_priority == LOG_PRIORITY_WARNING ||
	 _min_priority == LOG_PRIORITY_ERROR);
  if(running) {
    return -1;
  }
  if(pthread_mutex_init(&waiting_mutex, NULL)) {
    return -1;
  }
  if(pthread_mutex_init(&ready_mutex, NULL)) {
    pthread_mutex_destroy(&waiting_mutex);
    return -1;
  }
  if(pthread_cond_init(&waiting_cond, NULL)) {
    pthread_mutex_destroy(&ready_mutex);
    pthread_mutex_destroy(&waiting_mutex);
    return -1;
  }
  init_log_queue(&waiting);
  init_log_queue(&ready);
  output = _output;
  min_priority = _min_priority;
  running = true;
  if(pthread_create(&worker, NULL, log_messages, NULL)) {
    pthread_cond_destroy(&waiting_cond);
    pthread_mutex_destroy(&ready_mutex);
    pthread_mutex_destroy(&waiting_mutex);
    return -1;
  }
  return 0;
}

/**
 * Returns the minimum message priority
 */
enum log_priority get_min_priority() {
  return min_priority;
}

/**
 * Logs a message
 */
void log_msg(const char * file, int line, enum log_priority priority, const char * format, ...) {
  if(priority >= get_min_priority()) {
    struct log_msg * msg = get_ready_log_msg();
    assert(msg != NULL);
    msg->file = file;
    msg->line = line;
    msg->priority = priority;

    va_list args;
    va_start(args, format);
    int len = vsnprintf(msg->buffer, msg->cap, format, args);
    va_end(args);
    if(len < 0) {
      destroy_log_msg(msg);
      return;
    } else if((size_t)len >= msg->cap) {
      if(ensure_log_msg_buffer(msg, (size_t)len + 1)) {
	destroy_log_msg(msg);
	return;
      }
      va_list args2;
      va_start(args2, format);
      len = vsnprintf(msg->buffer, msg->cap, format, args2);
      if(len < 0){
	destroy_log_msg(msg);
	return;
      }
      va_end(args2);
      assert((size_t)len < msg->cap);
      msg->buffer[(size_t)len] = '\0';
    }
    if(pthread_mutex_lock(&waiting_mutex)) {
      destroy_log_msg(msg);
      return;
    }
    push_onto_log_queue(&waiting, msg);
    if(pthread_mutex_unlock(&waiting_mutex)) {
      return;
    }
    pthread_cond_signal(&waiting_cond);
  }
}

void log_error_code(const char * file, int line, const char * msg, int error_code) {
  assert(msg != NULL);
  char buffer[ERROR_CODE_BUFFER_SIZE];
  if(strerror_r(error_code, buffer, ERROR_CODE_BUFFER_SIZE)) {
    //soemthing went badly wrong
    log_msg(file, line, LOG_PRIORITY_ERROR, "%s: error code %d", msg, error_code);
  } else {
    log_msg(file, line, LOG_PRIORITY_ERROR, "%s: '%s'", msg, buffer);
  }
}

/**
 * Stops a logger
 */
void dispose_logger() {
  if(pthread_mutex_lock(&waiting_mutex)) {
    return;
  }
  running = false;
  if(pthread_mutex_unlock(&waiting_mutex)) {
    return;
  }
  if(pthread_cond_signal(&waiting_cond)) {
    return;
  }
  if(pthread_join(worker, NULL)) {
    return;
  }
  pthread_cond_destroy(&waiting_cond);
  pthread_mutex_destroy(&waiting_mutex);
  pthread_mutex_destroy(&ready_mutex);
  dispose_log_queue(&ready);
  dispose_log_queue(&waiting);
}
