#ifndef LOGGER_H
#define LOGGER_H

#include <errno.h>
#include <stdio.h>

/**
 * The priority of a log message
 */
enum log_priority {
  /**
   * Debug messages
   */
  LOG_PRIORITY_DEBUG,
  /**
   * Info messages
   */
  LOG_PRIORITY_INFO,
  /**
   * Non fatal errors
   */
  LOG_PRIORITY_WARNING,
  /**
   * Fatal errors
   */
  LOG_PRIORITY_ERROR
};

/**
 * Starts a logger
 */
int init_logger(FILE * file, enum log_priority min_priority);

/**
 * Returns the minimum message priority
 */
enum log_priority get_min_priority();

/**
 * Logs a message
 */
void log_msg(const char * file, int line, enum log_priority priority, const char * format, ...);

void log_error_code(const char * file, int line, const char * msg, int error_code);

/**
 * Stops a logger
 */
void dispose_logger();

#define LOG_MSG(priority, ...) if(priority >= get_min_priority()) log_msg(__FILE__, __LINE__, priority, __VA_ARGS__)

#define LOG_DEBUG(...) LOG_MSG(LOG_PRIORITY_DEBUG, __VA_ARGS__)

#define LOG_INFO(...) LOG_MSG(LOG_PRIORITY_INFO, __VA_ARGS__)

#define LOG_WARNING(...) LOG_MSG(LOG_PRIORITY_WARNING, __VA_ARGS__)

#define LOG_ERROR(...) LOG_MSG(LOG_PRIORITY_ERROR, __VA_ARGS__)

#define LOG_ERROR_CODE(msg, error_code) log_error_code(__FILE__, __LINE__, msg, error_code)

#define LOG_ERRNO(msg) LOG_ERROR_CODE(msg, errno)

#endif
