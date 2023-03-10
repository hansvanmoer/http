#include "url.h"

#include <assert.h>

void init_url_buffer(struct url_buffer * url) {
  assert(url != NULL);

  url->host = NULL;
  url->host_cap = 0;
  url->port = 80;
  url->path = NULL;
  url->path_cap = 0;
}

void dispose_url_buffer(struct url_buffer * url) {
  assert(url != NULL);

  free(url->host);
  free(url->path);
}
