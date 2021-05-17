/*******************************************************************************
 * http module for ncurest                                                     *
 *                                                                             *
 * structures for representing http requests and responses, methods for        *
 * parsing, dumping, sending and receiving http requests.                      *
 ******************************************************************************/

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
  size_t content_length;
} http_response;

/**
 * Dump a http_request to a dynamically allocated buffer, and store the pointer
 * to it in the given target buffer pointer.
 *
 * Arguments:
 *   req     http_request that will be dumped.
 *   target  pointer to a target buffer.
 *
 * Returns: The size of the buffer allocated to store the dumped request, or -1
 *          on error.
 */
int http_dump_request(http_request req, char **target);

/**
 * Parse a http_response from a buffer, dynamically allocating the
 * http_response, and storing it at the given target.
 *
 * Arguments:
 *   buffer  pointer to a buffer containing the http response.
 *   size:   the size of the buffer.
 *   target: pointer to store the resulting http response pointer in.
 *
 * Returns: The number of bytes consumed while parsing the http_response, or -1
 *          on error.
 */
int http_parse_response(char *buffer, size_t size, http_response **target);

/**
 * Send the given http_request on the socket with the file descriptor socket_fd.
 *
 * Arguments:
 *   socket_fd  file descriptor for a tcp socket.
 *   req        http_request to send on the socket.
 *
 * Returns: The number of bytes sent, or -1 on error.
 */
int http_send(int socket_fd, http_request req);

/**
 * Receive a http_response from a tcp socket with the file descriptor socket_fd.
 *
 * Arguments:
 *   socket_fd  file descriptor for the tcp socket.
 *
 * Returns: A pointer to the received http response, if one could be succesfully
 *          received, otherwise NULL.
 */
http_response *http_recv(int socket_fd);

/**
 * Free a http response.
 * 
 * Arguments:
 *   resp  pointer to the http_response to free.
 */
void http_response_free(http_response *resp);

#endif // __NCUREST_HTTP_H__
