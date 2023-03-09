#include <assert.h>
#include <stdlib.h>

#include "logger.h"
#include "task.h"

#define TASK_ERROR_BUF_LEN 128

/**
 * Creates a new task
 */
static struct task * create_task() {
  struct task * task = (struct task *) malloc(sizeof(struct task));
  if(task == NULL) {
    LOG_ERRNO("could not allocate memory for task");
    return NULL;
  }
  task->data = NULL;
  task->executor = NULL;
  task->destructor = NULL;
  return task;
}

/**
 * Destroys a task
 */
static void destroy_task(struct task * task) {
  assert(task != NULL);
  if(task->data != NULL && task->destructor != NULL) {
    (*task->destructor)(task->data);
  }
  free(task);
}

/**
 * Initializes a task queue
 */
static void init_task_queue(struct task_queue * q){
  assert(q != NULL);
  q->head = NULL;
  q->tail = NULL;
}

/**
 * Pushes a task onto a queue
 */
static void push_onto_task_queue(struct task_queue * q, struct task * t) {
  assert(q != NULL);
  assert(t != NULL);
  if(q->tail == NULL) {
    q->head = t;
    q->tail = t;
  } else {
    q->tail->next = t;
    q->tail = t;
  }
  t->next = NULL;
}

/**
 * Pops a task from a queue
 */
static struct task * pop_from_task_queue(struct task_queue * q) {
  assert(q != NULL);
  struct task * t = q->head;
  if(t != NULL){
    q->head = t->next;
    if(q->head == NULL) {
      q->tail = NULL;
    }
  }
 return t;
}

/**
 * Disposes of a task queue
 */
static void dispose_task_queue(struct task_queue * q) {
  assert(q != NULL);
  struct task * t = q->head;
  while(t != NULL) {
    struct task * n = t->next;
    destroy_task(t);
    t = n;
  }
}


/**
 * Get or creates a task that is ready to be used
 */
static struct task * get_ready_task(struct task_service * t) {
  assert(t != NULL);
  int result;
  if((result = pthread_mutex_lock(&t->ready_mutex))){
    LOG_ERROR_CODE("could not lock ready mutex", result);
    return NULL;
  }
  struct task * task = pop_from_task_queue(&t->ready);
  if((result = pthread_mutex_unlock(&t->ready_mutex))) {
    LOG_ERROR_CODE("could not unlock ready mutex", result);
    destroy_task(task);
    return NULL;
  }
  if(task == NULL) {
    task = create_task();
  }
  return task;
}

/**
 * Recycles a task
 */
static void recycle_task(struct task_service * t, struct task * task) {
  assert(t != NULL);
  assert(task != NULL);
  int result;
  if((result = pthread_mutex_lock(&t->ready_mutex))) {
    LOG_ERROR_CODE("could not unlock ready mutex", result);
    destroy_task(task);
    return;
  }
  push_onto_task_queue(&t->ready, task);
  if((result = pthread_mutex_unlock(&t->ready_mutex))) {
    LOG_ERROR_CODE("could not unlock ready mutex", result);
  }
} 

/**
 * Runs the task
 */
static void run_task(struct task_service * t, struct task * task) {
  assert(t != NULL);
  assert(task != NULL);
  (*task->executor)(task->data);
  if(task->destructor != NULL) {
    (*task->destructor)(task->data);
  }
}

/**
 * Runs the task server worker threads
 */
static void * run_task_worker(void * arg) {
  int result;
  struct task_service * t = (struct task_service *)arg;
  if((result = pthread_mutex_lock(&t->waiting_mutex))) {
    LOG_ERROR_CODE("could not lock waiting mutex", result);
      return NULL;
  }
  while(true) {
    if(t->running) {
      if((result = pthread_mutex_unlock(&t->waiting_mutex))) {
	LOG_ERROR_CODE("could not unlock waiting mutex", result);
      }
      return NULL;
    }
    struct task * task = pop_from_task_queue(&t->waiting);
    if(task == NULL) {
      if((result = pthread_cond_wait(&t->waiting_cond, &t->waiting_mutex))) {
	LOG_ERROR_CODE("could not wait on waiting condition variable", result);
	pthread_mutex_unlock(&t->waiting_mutex);
	return NULL;
      }
    } else {
      if((result = pthread_mutex_unlock(&t->waiting_mutex))) {
	LOG_ERROR_CODE("could not unlock waiting mutex", result);
	return NULL;
      }
      run_task(t, task);
      recycle_task(t, task);
      if((result = pthread_mutex_lock(&t->waiting_mutex))) {
	LOG_ERROR_CODE("could not lock waiting mutex", result);
	return NULL;
      }    
    }
  }
}

/*
 * The public API
 */

int init_task_service(struct task_service * t, size_t max_pool_size) {
  assert(t != NULL);
  if(max_pool_size == 0) {
    LOG_ERROR("max_pool_size must be > 0");
    return -1;
  }
  int result;
  if((result = pthread_mutex_init(&t->waiting_mutex, NULL))) {
    LOG_ERROR_CODE("could not initialize waiting mutex", result);
    return -1;
  }
  if((result = pthread_cond_init(&t->waiting_cond, NULL))) {
    LOG_ERROR_CODE("could not initialize waiting condition variable", result);
    pthread_mutex_destroy(&t->waiting_mutex);
    return -1;
  }
  if((result = pthread_mutex_init(&t->ready_mutex, NULL))) {
    LOG_ERROR_CODE("could not initialize ready mutex: %s", result);
    pthread_cond_destroy(&t->waiting_cond);
    pthread_mutex_destroy(&t->waiting_mutex);
    return -1;
  }
  t->workers = NULL;
  t->len = 0;
  t->cap = max_pool_size;
  t->running = false;
  init_task_queue(&t->waiting);
  init_task_queue(&t->ready);
  return 0;
}

int start_task_service(struct task_service * t) {
  assert(t != NULL);
  pthread_t * workers = (pthread_t *)malloc(sizeof(pthread_t) * t->cap);
  if(workers == NULL) {
    LOG_ERROR("worker thread buffer allocation failed");
    return -1;
  }
  t->workers = workers;
  t->len = 0;
  t->running = true;
  int result = 0;
  for(size_t i = 0; i < t->cap; ++i) {
    result = pthread_create(t->workers + i, NULL, run_task_worker, t);
    if(result != 0) {
      LOG_ERROR_CODE("could not create worker thread", result);
      break;
    }
    ++t->len;
  }
  if(result != 0) {
    //something went wrong
    stop_task_service(t);
    return result;
  }
  return 0;
}

int add_task(struct task_service * t, void (*executor)(void *), void * data, void (*destructor)(void *)) {
  assert(t != NULL);
  struct task * task = get_ready_task(t);
  if(task == NULL) {
    return -1;
  }
  task->executor = executor;
  task->data = data;
  task->destructor = destructor;
  int result;
  if((result = pthread_mutex_lock(&t->waiting_mutex))) {
    LOG_ERROR_CODE("could not lock waiting mutex", result);
    destroy_task(task);
    return -1;
  }
  push_onto_task_queue(&t->waiting, task);
  if((result = pthread_mutex_unlock(&t->waiting_mutex))) {
  LOG_ERROR_CODE("could not unlock waiting mutex", result);
    return -1;
  }
  if((result = pthread_cond_signal(&t->waiting_cond))) {
    LOG_ERROR_CODE("could not signal waiting condition variable", result);
    return -1;
  }
  return 0;
}

void stop_task_service(struct task_service * t) {
  assert(t != NULL);
  int result;
  if((result = pthread_mutex_lock(&t->waiting_mutex))) {
    LOG_ERROR_CODE("could not lock waiting mutex", result);
    return;
  }
  t->running = false;
  if((result = pthread_cond_broadcast(&t->waiting_cond))) {
    LOG_ERROR_CODE("could not broadcast to waiting condition variable", result);
    pthread_mutex_unlock(&t->waiting_mutex);
    return;
  }
  if((result = pthread_mutex_unlock(&t->waiting_mutex))) {
    LOG_ERROR_CODE("could not unlock waiting mutex", result);
    return;
  }
  for(size_t i = 0; i < t->len; ++i) {
    pthread_join(t->workers[i], NULL);
  }
  free(t->workers);
  t->workers = NULL;
  t->len = 0;
  t->cap = 0;
}

void dispose_task_service(struct task_service * t) {
  assert(t != NULL);
  dispose_task_queue(&t->ready);
  dispose_task_queue(&t->waiting);
  int result;
  if((result = pthread_cond_destroy(&t->waiting_cond))) {
    LOG_ERROR_CODE("could not destroy waiting condition variable", result);
  }
  if((result = pthread_mutex_destroy(&t->waiting_mutex))) {
    LOG_ERROR_CODE("could not destroy waiting mutex", result);
  }
  if((result = pthread_mutex_destroy(&t->ready_mutex))) {
    LOG_ERROR_CODE("could not destroy ready mutex", result);
  }
}
