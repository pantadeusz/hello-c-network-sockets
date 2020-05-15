/**
 * @author Tadeusz Puźniakowski
 * @copyright Copyright (c) 2019 Tadeusz Puźniakowski
 * @license MIT
 */

// see also http://man7.org/linux/man-pages/man3/getaddrinfo.3.html

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int connected_socket_ready(int connected_socket) {
  char *msg;
  int msgsize;
  read(connected_socket, &msgsize, sizeof(int));
  msg = (char *)malloc(msgsize + 1);
  read(connected_socket, msg, msgsize);
  msg[msgsize] = 0;
  printf("received \"%s\"\n", msg);
  free(msg);
  close(connected_socket); // close connected socket
  return 0;
}

int listening_socket_ready(int listening_socket) {
  struct sockaddr_storage peer_addr;
  socklen_t peer_addr_len = sizeof(struct sockaddr_storage);
  int connected_socket =
      accept(listening_socket, (struct sockaddr *)&peer_addr, &peer_addr_len);

  if (connected_socket == -1) {
    fprintf(stderr, "could not accept connection!\n");
    return connected_socket;
  }

  char host_[NI_MAXHOST], service_[NI_MAXSERV];
  getnameinfo((struct sockaddr *)&peer_addr, peer_addr_len, host_, NI_MAXHOST,
              service_, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST);
  printf("Accepted connection from: %s:%s\n", host_, service_);
  connected_socket_ready(connected_socket);

  close(listening_socket); // stop listening on socket
  return 0;
}

int main(int argc, char **argv) {
  const int max_queue = 32;
  int listening_socket;
  struct addrinfo hints;
  bzero((char *)&hints, sizeof(struct addrinfo));

  hints.ai_family = AF_UNSPEC;     ///< IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; ///< Stream socket
  hints.ai_flags = AI_PASSIVE;     ///< For wildcard IP address

  struct addrinfo *result, *rp;
  int s = getaddrinfo((argc >= 2) ? argv[1] : NULL, (argc >= 3) ? argv[2] : "9090",
                      &hints, &result);
  if (s != 0) {
    fprintf(stderr, gai_strerror(s));
    return -1;
  }

  // try to create socket and bind it to address
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    listening_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (listening_socket != -1) {
      int yes = 1;
      if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &yes,
                     sizeof(yes)) == -1) {
        fprintf(stderr, "setsockopt( ... ) error\n");
        return -1;
      }
      if (bind(listening_socket, rp->ai_addr, rp->ai_addrlen) == 0) {
        freeaddrinfo(result);
        if (listen(listening_socket, max_queue) == -1) {
          fprintf(stderr, "listen error\n");
          return -1;
        }
        return listening_socket_ready(listening_socket);
      }
      close(listening_socket); // didn't work, let's close socket
    }
  }
  freeaddrinfo(result);
  fprintf(stderr, "error binding adress\n");
  return -1;
}
