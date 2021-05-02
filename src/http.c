#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "http.h"

size_t http_dump_request(http_request req, char **dump) {
  char *allocated;
  size_t size, offset;

  // compute the size of the memory to allocate
  size = snprintf(NULL, 0, "%s %s HTTP/1.1\r\n", req.method, req.uri);
  for (int i = 0; i < req.header_count; i++) {
    size += snprintf(NULL, 0, "%s: %s\r\n", req.headers[i].header,
                     req.headers[i].value);
  }

  // last crln
  size += 2;

  allocated = malloc(sizeof(*allocated) * (size + 1));

  // check if memory was allocated
  if (allocated == NULL) {
    return 0;
  }

  offset = sprintf(allocated, "%s %s HTTP/1.1\r\n", req.method, req.uri);
  for (int i = 0; i < req.header_count; i++) {
    offset += sprintf(&allocated[offset], "%s: %s\r\n", req.headers[i].header,
                      req.headers[i].value);
  }
  sprintf(&allocated[offset], "\r\n");

  *dump = allocated;

  return size;
}

char *take_until(char *ptr, char *end, char **ret, char c) {
  char *result;
  size_t i, size = 0;

  while (ptr < end && *ptr != c) {
    size++;
    ptr++;
  }

  if (size == 0) {
    return NULL;
  }

  // allocate memory for http version
  result = malloc(sizeof *result * (size + 1));
  if (result == NULL) {
    return (char *)NULL;
  }

  // copy the http version
  for (i = 0; i < size; i++) {
    result[i] = (ptr - size)[i];
  }

  *ret = ptr;
  result[size] = 0;

  return result;
}

char *skip(char *ptr, char *end, size_t nskip, const char *skip) {
  size_t i;

  for (i = 0; i < nskip; i++) {
    if (ptr + i >= end || ptr[i] != skip[i]) {
      return (char *)NULL;
    }
  }

  return &ptr[nskip];
}

void http_response_free(http_response *resp) {
  size_t i;

  if (resp->http_version != NULL) {
    free(resp->http_version);
  }

  if (resp->headers != NULL) {
    for (i = 0; i < resp->header_count; i++) {
      free(resp->headers[i].header);
      free(resp->headers[i].value);
    }

    free(resp->headers);
  }
}

http_response *http_parse_response(void *buffer, size_t size) {
  http_response *resp;
  char *ptr = (char *)buffer;
  char *end = (char *)(ptr + size);
  http_header *headers_tmp;
  size_t i, headers_allocated;

  // allocate respones memory
  resp = malloc(sizeof *resp);

  // parse http version
  resp->http_version = take_until(ptr, end, &ptr, ' ');
  if (resp->http_version == NULL) {
    free(resp);
    return (http_response *)NULL;
  }

  // skip the space or fail
  ptr = skip(ptr, end, 1, " ");
  if (ptr == NULL || ptr == end) {
    http_response_free(resp);
    return (http_response *)NULL;
  }

  // parse status code
  resp->status_code = (int)strtol(ptr, &ptr, 10);

  // skip the space or fail
  ptr = skip(ptr, end, 1, " ");
  if (ptr == NULL || ptr == end) {
    http_response_free(resp);
    return (http_response *)NULL;
  }

  // parse reason phrase
  resp->reason_phrase = take_until(ptr, end, &ptr, '\r');
  if (resp->reason_phrase == NULL) {
    http_response_free(resp);
    return (http_response *)NULL;
  }

  // skip carriage return and line feed
  ptr = skip(ptr, end, 2, "\r\n");
  if (ptr == NULL || ptr == end) {
    http_response_free(resp);
    return (http_response *)NULL;
  }

  // parse headers
  resp->header_count = 0;
  headers_allocated = 5;
  resp->headers = malloc(sizeof *resp->headers * headers_allocated);
  if (resp->headers == NULL) {
    http_response_free(resp);
    return (http_response *)NULL;
  }

  while (ptr < end && *ptr != '\r') {
    // allocate
    if (resp->header_count == headers_allocated) {
      headers_allocated = headers_allocated * 2;
      headers_tmp =
          realloc(resp->headers, sizeof *resp->headers * headers_allocated);
      if (resp->headers == NULL) {
        http_response_free(resp);
        return (http_response *)NULL;
      } else {
        resp->headers = headers_tmp;
      }
    }

    // header name
    resp->headers[resp->header_count].header = take_until(ptr, end, &ptr, ':');
    ptr = skip(ptr, end, 2, ": ");

    if (resp->headers[resp->header_count].header == NULL || ptr == NULL ||
        ptr == end) {
      http_response_free(resp);
      return (http_response *)NULL;
    }

    // header value
    resp->headers[resp->header_count].value = take_until(ptr, end, &ptr, '\r');
    ptr = skip(ptr, end, 2, "\r\n");
    if (resp->headers[resp->header_count].value == NULL || ptr == NULL ||
        ptr == end) {
      http_response_free(resp);
      return (http_response *)NULL;
    }
    resp->header_count++;
  }

  // check ending carriage return and newline
  ptr = skip(ptr, end, 2, "\r\n");
  if (ptr == NULL) {
    http_response_free(resp);
    return (http_response *)NULL;
  }

  // set the body pointer
  resp->body = ptr < end ? (void *)ptr : NULL;

  return resp;
}

http_response *http_send_request(const char *host, const char *port,
                                 http_request req) {
  int socket_fd;
  char *send_buf, *recv_buf, *recv_tmp;
  size_t send_len, recv_len, recv_res;
  http_response *resp;
  struct addrinfo hints, *results, *res;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_flags = AI_V4MAPPED;

  if (getaddrinfo(host, port, &hints, &results) != 0) {
    return (http_response *)NULL;
  }

  for (res = results; res != NULL; res = res->ai_next) {
    socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (socket_fd == -1) {
      continue;
    }

    if (connect(socket_fd, res->ai_addr, res->ai_addrlen) != -1) {
      break;
    }

    close(socket_fd);
  }

  freeaddrinfo(results);

  if (res == NULL) {
    return (http_response *)NULL;
  }

  send_len = http_dump_request(req, &send_buf);
  if (send_buf == NULL) {
    return (http_response *)NULL;
  }

  // by this point we have opened a tcp connection
  if (send(socket_fd, send_buf, send_len, 0) == -1) {
    return NULL;
  }

  recv_buf = malloc(HTTP_RECV_BUFSIZE * sizeof *recv_buf);
  recv_len = 0;
  while ((recv_res =
              recv(socket_fd, &recv_buf[recv_len], HTTP_RECV_BUFSIZE, 0)) > 0) {
    recv_len += recv_res;
    recv_tmp =
        realloc(recv_buf, (recv_len + HTTP_RECV_BUFSIZE) * sizeof *recv_buf);

    if (recv_tmp == NULL) {
      // handle error
    }

    recv_buf = recv_tmp;
  }

  if (recv_res == -1) {
    // error
    return NULL;
  }

  resp = http_parse_response(recv_buf, recv_len);

  return resp;
}
