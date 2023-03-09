#ifndef TASK_H
#define TASK_H

#include <stdbool.h>
#include <stdlib.h>

#include <pthread.h>

/**
 * A task definition
 */
struct task {
  /**
   * The data to be passed to the executor
   */
  void * data;
  /**
   * A destructor for the data
   */
  void (*destructor)(void *);

  /**
   * The executor function
   */
  void (*executor)(void *);
  /**
   * The next task in the list
   */
  struct task * next;
};

/**
 * A queue of tasks
 */
struct task_queue {
  /**
   * The first task in the queue
   */
  struct task * head;
  /**
   * The last task in the queue
   */
  struct task * tail;
};

/**
 * A task service implemented as a fixed thread pool
 */
struct task_service {
  /**
   * The available worker threads
   */
  pthread_t * workers;
  /**
   * The number of active threads
   */
  size_t len;
  /**
   * The maximum number of threads
   */
  size_t cap;
  /**
   * Whether the service is running
   */
  bool running;
  /**
   * Tasks waiting to be done
   */
  struct task_queue waiting;
  /**
   * A mutex for the waiting queue
   */
  pthread_mutex_t waiting_mutex;
  /**
   * A condition variable for waiting queue
   */
  pthread_cond_t waiting_cond;
  /**
   * A queue to reuse tasks that are done
   */
  struct task_queue ready;
  /**
   * A mutex for the ready queue
   */
  pthread_mutex_t ready_mutex;
};

/**
 * Initializes the task service
 */
int init_task_service(struct task_service * t, size_t max_pool_size);

/**
 * Starts the task service
 */
int start_task_service(struct task_service * t);

/**
 * Creates a new task and adds it to the task server's queue
 */
int add_task(struct task_service * t, void (*executor)(void *), void * data, void (*destructor)(void *));

/**
 * Closes the task service
 */
void stop_task_service(struct task_service * t);

/**
 * Disposes of the task service
 */
void dispose_task_service(struct task_service * t);

#endif
