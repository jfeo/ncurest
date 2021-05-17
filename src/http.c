#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "http.h"
#include "net.h"

int http_dump_request(http_request req, char **target) {
  char *allocated;
  int size, offset;

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

  *target = allocated;

  return size;
}

/**
 * Take characters from the given source buffer until the given character, and
 * store the taken characters in a dynamically allocated buffer.
 *
 * Arguments:
 *   src      the source buffer to take from.
 *   dest     uninitialized pointer to the destination buffer that will be
 *            allocated.
 *   offset   inclusive index in the source buffer from which to start taking.
 *   bufsize  size of the source buffer.
 *   until_c  search character that will not be taken.
 *
 * Returns: The number of characters taken, or -1 on error.
 */
int take_until(char *src, char **dest, int offset, int bufsize, char until_c) {
  size_t taken = 0;

  while (offset + taken < bufsize && src[offset + taken] != until_c) {
    taken++;
  }

  if (taken == 0) {
    *dest = NULL;
    return 0;
  }

  // allocate memory for the taken characters
  *dest = malloc(sizeof **dest * (taken + 1));
  if (*dest == NULL) {
    return -1;
  }

  // copy
  strncpy(*dest, &src[offset], taken);
  (*dest)[taken] = '\0';

  return taken;
}

/**
 * Skips specific characters in the buffer from the given offset.
 *
 * Arguments:
 *   buf     buffer to skip in.
 *   offset  the index in the buffer to skip from.
 *   n       the number of characters to skip.
 *   skip    the order of characters to skip.
 *
 * Returns: n if successful, otherwise -1.
 */
int skip(char *buf, int offset, int bufsize, int n, const char *skip) {
  size_t i;

  for (i = 0; i < n; i++) {
    if (offset + i >= bufsize || buf[offset + i] != skip[i]) {
      return -1;
    }
  }

  return n;
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

int http_parse_response(char *buffer, size_t size, http_response **target) {
  http_response *resp;
  size_t offset = 0;
  http_header *headers_tmp;
  size_t headers_allocated;
  char *ptr;

  *target = NULL;

  // allocate respones memory
  resp = malloc(sizeof *resp);
  resp->content_length = 0;
  resp->body = NULL;
  resp->header_count = 0;
  resp->http_version = NULL;
  resp->reason_phrase = NULL;
  resp->headers = NULL;
  resp->status_code = -1;

  // parse http version
  offset += take_until(buffer, &resp->http_version, offset, size, ' ');
  if (resp->http_version == NULL) {
    http_response_free(resp);
    return -1;
  }

  // skip the space or fail
  if (skip(buffer, offset, size, 1, " ") == -1) {
    http_response_free(resp);
    return -1;
  }
  offset += 1;

  // parse status code
  resp->status_code = (int)strtol(&buffer[offset], &ptr, 10);
  offset = ptr - buffer;

  // skip the space or fail
  if (skip(buffer, offset, size, 1, " ") == -1) {
    http_response_free(resp);
    return -1;
  }
  offset += 1;

  // parse reason phrase
  offset += take_until(buffer, &resp->reason_phrase, offset, size, '\r');
  if (resp->reason_phrase == NULL) {
    http_response_free(resp);
    return -1;
  }

  // skip carriage return and line feed
  if (skip(buffer, offset, size, 2, "\r\n") == -1) {
    http_response_free(resp);
    return -1;
  }
  offset += 2;

  // prepare header parsing
  resp->header_count = 0;
  headers_allocated = 5;
  resp->headers = malloc(sizeof *resp->headers * headers_allocated);
  if (resp->headers == NULL) {
    http_response_free(resp);
    return -1;
  }

  // parse headers
  while (offset < size && buffer[offset] != '\r') {
    // allocate
    if (resp->header_count == headers_allocated) {
      headers_allocated = headers_allocated * 2;
      headers_tmp =
          realloc(resp->headers, sizeof *resp->headers * headers_allocated);
      if (resp->headers == NULL) {
        http_response_free(resp);
        return -1;
      } else {
        resp->headers = headers_tmp;
      }
    }

    // take header name
    offset += take_until(buffer, &resp->headers[resp->header_count].header,
                         offset, size, ':');

    if (resp->headers[resp->header_count].header == NULL ||
        skip(buffer, offset, size, 2, ": ") == -1) {
      http_response_free(resp);
      return -1;
    }
    offset += 2;

    // header value
    offset += take_until(buffer, &resp->headers[resp->header_count].value,
                         offset, size, '\r');
    if (resp->headers[resp->header_count].value == NULL ||
        skip(buffer, offset, size, 2, "\r\n") == -1) {
      http_response_free(resp);
      return -1;
    }
    offset += 2;

    // specific header logic
    if (strcmp("Content-Length", resp->headers[resp->header_count].header) ==
        0) {
      char *endptr;
      resp->content_length =
          (size_t)strtoul(resp->headers[resp->header_count].value, &endptr, 10);
      if (*endptr != '\0') {
        resp->content_length = 0;
      }
    }

    resp->header_count++;
  }

  // check ending carriage return and newline
  if (skip(buffer, offset, size, 2, "\r\n") == -1) {
    http_response_free(resp);
    return -1;
  }
  offset += 2;

  // set the body pointer
  resp->body = offset < size ? (void *)&buffer[offset] : NULL;

  // set the target
  *target = resp;

  return offset;
}

int http_send(int socket_fd, http_request req) {
  char *buf;
  size_t len;

  len = http_dump_request(req, &buf);
  if (buf == NULL) {
    return -1;
  }

  return send(socket_fd, buf, len, 0);
}

http_response *http_recv(int socket_fd) {
  char *buf, *tmp;
  size_t len, res, offset, remaining;
  http_response *resp = NULL;

  buf = malloc(HTTP_RECV_BUFSIZE * sizeof *buf);
  len = 0;
  while ((res = recv(socket_fd, &buf[len], HTTP_RECV_BUFSIZE, 0)) > 0) {
    len += res;
    tmp = realloc(buf, (len + HTTP_RECV_BUFSIZE) * sizeof *buf);

    if (tmp == NULL) {
      return NULL;
    }

    buf = tmp;

    if (resp == NULL) {
      offset = http_parse_response(buf, len, &resp);
      if (resp != NULL) {
        remaining = resp->content_length - (len - offset);
      }
    } else {
      remaining -= res;
    }

    if (remaining == 0) {
      break;
    }
  }

  if (res == -1) {
    return NULL;
  }

  return resp;
}
