#include "buffer.h"
#include "logger.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>

#define TEXT_BUFFER_INITIAL_CAP 256
#define TEXT_BUFFER_GROWTH_FACTOR 2

/**
 * Grows a text buffer
 */
static int grow_text_buffer(struct text_buffer * b) {
  assert(b != NULL);

  size_t ncap = b->cap == 0 ? TEXT_BUFFER_INITIAL_CAP : b->cap * TEXT_BUFFER_GROWTH_FACTOR;
  void * ndata = realloc(b->data, ncap);
  if(ndata == NULL) {
    LOG_ERRNO("could not allocate memory");
    return -1;
  }
  b->data = ndata;
  b->cap = ncap;
  return 0;
}

void init_text_buffer(struct text_buffer * b) {
  assert(b != NULL);
  
  b->data = NULL;
  b->len = 0;
  b->cap = 0;
}

void clear_text_buffer(struct text_buffer * b) {
  assert(b != NULL);
  b->len = 0;
}

void securely_clear_text_buffer(struct text_buffer * b) {
  assert(b != NULL);
  
  memset(b->data, 0, b->cap);
  b->len = 0;
}

int remove_char(struct text_buffer * b) {
  assert(b != NULL);
  if(b->len == 0) {
    return -1;
  } else {
    --b->len;
  }
}

int append_char(struct text_buffer * b, char c) {
  assert(b != NULL);

  if(b->len == b->cap) {
    if(grow_text_buffer(b)) {
      return -1;
    }
  }

  b->data[b->len] = c;
  ++b->len;
  return 0;
}

int read_char(struct text_buffer * b, int fd, size_t max) {
  assert(b != NULL);

  if(b->len >= max) {
    errno = E2BIG;
    return -1;
  }
  
  if(b->len == b->cap) {
    if(grow_text_buffer(b)) {
      return -1;
    }
  }
  ssize_t result = read(fd, b->data + b->len, 1);
  if(result != 1) {
    return -1;
  }
  ++b->len;
  return 0;
}

int read_until_char(struct text_buffer * b, int fd, char delim, size_t max) {
  assert(b != NULL);

  while(b->len < max) {
    if(b->len == b->cap) {
      if(grow_text_buffer(b)) {
	return -1;
      }
    }
    ssize_t result = read(fd, b->data + b->len, 1);
    if(result != 1) {
      return -1;
    }
    if(b->data[b->len] == delim) {
      ++b->len;
      return 0;
    }
    ++b->len;
  }
  errno = E2BIG;
  return -1;
}

int read_until_string(struct text_buffer * b, int fd, const char * delim, size_t max) {
  assert(b != NULL);
  assert(delim != NULL);

  // zero length delimiter is not allowed
  if(*delim == '\0') {
    errno = EINVAL;
    return -1;
  }
  
  while(true) {
    if(read_until_char(b, fd, *delim, max)) {
      return -1;
    }
    const char * d = delim + 1;
    while(*d != '\0'){
      if(read_char(b, fd, max)) {
	return -1;
      }
      char c = b->data[b->len - 1]; 
      if(c != *d){
	if(c == *delim) {
	  d = delim;
	} else {
	  break;
	}
      }
      ++d;
    }
    if(*d == '\0') {
      break;
    }
  }
  return 0;
}

void dispose_text_buffer(struct text_buffer * b) {
  assert(b != NULL);
  
  free(b->data);
}
