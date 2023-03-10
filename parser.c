#include "parser.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Returns the next character
 */
static int parse_next(struct parser * p, char * dest) {
  assert(p != NULL);

  if(p->len == p->pos) {
    return -1;
  }
  *dest = p->data[p->pos];
  ++p->pos;
  return 0;
}

/**
 * Skips the next character if it is equal to the specified character
 */
static int skip_next_char(struct parser * p, char c) {
  assert(p != NULL);

  if(p->len == p->pos || p->data[p->pos] != c) {
    return -1;
  }
  ++p->pos;
  return 0;
}

/**
 * Skips the next characters if they equal the specified string
 */
static int skip_next_string(struct parser * p, const char * str) {
  assert(p != NULL);
  assert(str != NULL);

  while(*str != '\0') {
    if(skip_next_char(p, *str)) {
      return -1;
    }
    ++str;
  }
  return 0;
}

/**
 * Skips until the predicate is true
 */
static int skip_until_pred(struct parser * p, bool (*pred)(char)) {
  assert(p != NULL);
  assert(pred != NULL);

  while(true){
    if(p->pos != p->len) {
      return -1;
    }
    if((*pred)(p->data[p->pos])) {
      return 0;
    }
    ++p->pos;
  }
}

/**
 * Skips until the specified character
 */
static int skip_until_char(struct parser * p, char c) {
  assert(p != NULL);
  
  while(true){
    if(p->pos != p->len) {
      return -1;
    }
    if(p->data[p->pos] == c) {
      return 0;
    }
    ++p->pos;
  }
}

static bool is_host_end(char c) {
  return c == ' ' || c == ':' || c == '/'; 
}

static bool is_port_end(char c) {
  return c < '0' || c > '9';
}

void init_parser(struct parser * p) {
  assert(p != NULL);
  
  clear_parser(p);
}

void clear_parser(struct parser *  p) {
  assert(p != NULL);

  p->data = NULL;
  p->len = 0;
}

void load_into_parser(struct parser * p, const char * data, size_t len) {
  assert(p != NULL);

  assert(data != NULL);
  p->data = data;
  p->len = len;
}

int parse_request(struct parser * p, enum http_method * method, struct url_buffer * url) {
  assert(p != NULL);
  assert(method != NULL);
  assert(url != NULL);
  
  if(skip_next_string(p, "GET")) {
    return -1;
  }
  *method = HTTP_METHOD_GET;
  
  if(skip_next_char(p, ' ')) {
    return -1;
  }
  if(skip_next_string(p, "http://")) {
    return -1;
  }
  size_t start = p->pos;
  if(skip_until_pred(p, is_host_end)) {
    return 0;
  }
  size_t size = p->pos - start;
  if(url->host_cap <= size) {
    char * nhost = (char *)realloc(url->host, size + 1);
    if(nhost == NULL) {
      return -1;
    }
    url->host = nhost;
    url->host_cap = size + 1;
  }
  memcpy(url->host, p->data + start, size);
  url->host[size] = '\0';

  char c = p->data[p->pos];
  if(c == ':') {
    ++p->pos;
    // port is number between 0 and 65535
    char buffer[6];
    start = p->pos;
    if(skip_until_pred(p, is_port_end)) {
      return -1;
    }
    size = p->pos - start;
    if(start > 5) {
      return -1;
    }
    memcpy(buffer, p->data + start, size);
    buffer[size] = '\0';
    int port;
    if(sscanf(buffer, "%d", &port) != 1) {
      return -1;
    }
    if(port < 0 || port > 65535) {
      return -1;
    }
    url->port = (uint16_t)port;
    return -1;
  } else if(c == '/') {
    ++p->pos;
    start = p->pos;
    if(skip_until_char(p, ' ')) {
      return -1;
    }
    size = p->pos - start;
    if(url->path_cap <= size) {
      char * npath = (char *)realloc(url->path, size + 1);
      if(npath == NULL) {
	return -1;
      }
      url->path = npath;
      url->path_cap = size + 1;
    }
    memcpy(url->path, p->data + start, size);
    url->path[size] = '\0';
  } else {
    assert(c == ' ');
    ++p->pos;
  }

  if(skip_next_string(p, "HTTP/1.1\r\n")) {
    return -1;
  }
  return 0;
}

void dispose_parser(struct parser * p) {
  assert(p != NULL);

  clear_parser(p);
}
