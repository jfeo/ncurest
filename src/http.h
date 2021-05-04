#ifndef __NCUREST_HTTP_H__
#define __NCUREST_HTTP_H__

#include <stdlib.h>

#define HTTP_RECV_BUFSIZE 512 * 8

typedef struct {
  char *header;
  char *value;
} http_header;

typedef struct {
  const char *uri;
  const char *method;
  size_t header_count;
  http_header *headers;
} http_request;

typedef struct {
  char *http_version;
  int status_code;
  char *reason_phrase;
  size_t header_count;
  http_header *headers;
  void *body;
} http_response;

size_t http_dump_request(http_request req, char **buf);

http_response *http_parse_response(void *buffer, size_t size);

int http_send(int socket_fd, http_request req);

http_response *http_recv(int socket_fd);

#endif // __NCUREST_HTTP_H__
