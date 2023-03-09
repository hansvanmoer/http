#ifndef BUFFER_H
#define BUFFER_H

#include <stdlib.h>

/**
 * A text buffer
 */
struct text_buffer {
  /**
   * The data
   */
  char * data;
  /**
   * The number of bytes in the buffer
   */
  size_t len;
  /**
   * The capacity of the buffer
   */
  size_t cap;
};

/**
 * initializes a text buffer
 */
void init_text_buffer(struct text_buffer * b);

/**
 * Clears the buffer
 */
void clear_text_buffer(struct text_buffer * b);

/**
 * Clears the buffer, overwriting any text in it
 */
void securely_clear_text_buffer(struct text_buffer * buffer);

/**
 * Appends a character to the text buffer
 */
int append_char(struct text_buffer * b, char c);

/**
 * Reads a single character into the buffer
 */
int read_char(struct text_buffer * b, int fd, size_t max);

/**
 * Reads data into the buffer until the specified delimiter is encountered
 * The delimiter is appended to the buffer
 */
int read_until_char(struct text_buffer * b, int fd, char delim, size_t max);

/**
 * Reads data into the buffer until the specified delimiter is encountered
 * The delimiter is appended to the buffer
 */
int read_until_string(struct text_buffer * b, int fd, const char * delim, size_t max);

/**
 * Disposes of the text buffer
 */
void dispose_text_buffer(struct text_buffer * b);

#endif
