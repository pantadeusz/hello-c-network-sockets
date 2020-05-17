/**
 * @author Tadeusz Puźniakowski
 * @copyright Copyright (c) 2019 Tadeusz Puźniakowski
 * @license MIT
 */

// see also http://man7.org/linux/man-pages/man3/getaddrinfo.3.html

#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int connected_socket_ready(int connected_socket) {
  char *request;
  int request_size = 0;
  request = (char *)malloc(request_size + 1);
  while (read(connected_socket, request + request_size, 1) > 0) {
    printf("%c", request + request_size);
    request_size++;
    request[request_size] = 0;
    request = (char *)realloc(request, request_size + 1);
    if (request_size > 4) {
      if (strcmp("\r\n\r\n", request + request_size - 4) == 0) {
        printf("Zapytanie:\n%s\n", request);
        break;
      }
    }
  }
  char *response =
      "HTTP/1.1 200 OK\r\n"
      "Date: Mon, 23 May 2005 22:38:34 GMT\r\n"
      "Content-Type: text/html; charset=UTF-8\r\n"
      "Content-Length: 155\r\n"
      "Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT\r\n"
      "Server: Apache/1.3.3.7 (Unix) (Red-Hat/Linux)\r\n"
      "Accept-Ranges: bytes\r\n"
      "Connection: close\r\n"
      "\r\n"
      "<html>\r\n"
      "  <head>\r\n"
      "    <title>An Example Page</title>\r\n"
      "  </head>\r\n"
      "  <body>\r\n"
      "    <p>Hello World, this is a very simple HTML document.</p>\r\n"
      "  </body>\r\n"
      "</html>\r\n";

  write(connected_socket, response, strlen(response));
  free(request);
  close(connected_socket); // close connected socket
  return 0;
}

volatile int still_working = 1;

void on_finito(int sig) { still_working = 0; }

int listening_socket_ready(int listening_socket) {
  struct sockaddr_storage peer_addr;
  socklen_t peer_addr_len = sizeof(struct sockaddr_storage);
  while (still_working) {
    int connected_socket =
        accept(listening_socket, (struct sockaddr *)&peer_addr, &peer_addr_len);

    if (connected_socket == -1) {
      fprintf(stderr, "could not accept connection!\n");
      continue;
      //      return connected_socket;
    }

    char host_[NI_MAXHOST], service_[NI_MAXSERV];
    getnameinfo((struct sockaddr *)&peer_addr, peer_addr_len, host_, NI_MAXHOST,
                service_, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST);
    printf("Accepted connection from: %s:%s\n", host_, service_);
    connected_socket_ready(connected_socket);
  }
  printf("koniec\n");
  close(listening_socket); // stop listening on socket
  return 0;
}

int main(int argc, char **argv) {
  signal(SIGINT, on_finito);
  const int max_queue = 32;
  int listening_socket;
  struct addrinfo hints;
  bzero((char *)&hints, sizeof(struct addrinfo));

  hints.ai_family = AF_UNSPEC;     ///< IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; ///< Stream socket
  hints.ai_flags = AI_PASSIVE;     ///< For wildcard IP address

  struct addrinfo *result, *rp;
  int s = getaddrinfo((argc >= 2) ? argv[1] : NULL,
                      (argc >= 3) ? argv[2] : "9090", &hints, &result);
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
