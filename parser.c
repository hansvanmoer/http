#include "parser.h"

#include <assert.h>
#include <stdlib.h>

void init_parser(struct parser * p) {
  assert(p != NULL);
  
  clear_parser(p);
}

/**
 * Resets the parser
 */
void clear_parser(struct parser *  p) {
  assert(p != NULL);

  p->data = NULL;
  p->len = 0;
}

/**
 * Loads data into the parser
 */
void load_into_parser(struct parser * p, const char * data, size_t len) {
  assert(p != NULL);

  assert(data != NULL);
  p->data = data;
  p->len = len;
}

/**
 * Disposes of a parser
 */
void dispose_parser(struct parser * p) {
  assert(p != NULL);

  clear_parser(p);
}
