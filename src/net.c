#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "net.h"

int net_tcp_connect(const char *host, const char *port) {
  int socket_fd;
  struct addrinfo hints, *results, *res;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;

  if (getaddrinfo(host, port, &hints, &results) != 0) {
    return -1;
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

  int one = 1;
  if (setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one) != 0) {
    close(socket_fd);
    return -1;
  }

  return socket_fd;
}
