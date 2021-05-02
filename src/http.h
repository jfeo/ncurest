#ifndef __NCURLSES_HTTP_H__
#define __NCURLSES_HTTP_H__

#include <stdlib.h>

#define HTTP_RECV_BUFSIZE 512

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
http_response *http_send_request(const char *host, const char *port, http_request req); 

#endif
