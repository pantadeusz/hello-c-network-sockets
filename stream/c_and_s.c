/**
 * @file client.cpp
 * @author Tadeusz Puźniakowski
 * @brief Basic client
 * @version 0.1
 * @date 2019-01-16
 *
 * @copyright Copyright (c) 2019 Tadeusz Puźniakowski
 * @license MIT
 */

#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief opens a connection to given host and port
 *
 * @param addr_txt
 * @param port_txt
 *
 * @return connected socket
 */
int connect_to(const char *addr_txt, const char *port_txt) {
  struct addrinfo hints;
  bzero((char *)&hints, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;     ///< IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; ///< stream socket

  struct addrinfo *addr_p;
  int err = getaddrinfo(addr_txt, port_txt, &hints, &addr_p);
  if (err) {
    fprintf(stderr,gai_strerror(err));
    return -1;
  }
  struct addrinfo *rp;
  // find first working address that we can connect to
  for (rp = addr_p; rp != NULL; rp = rp->ai_next) {
    int connected_socket =
        socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (connected_socket != -1) {
      if (connect(connected_socket, rp->ai_addr, rp->ai_addrlen) != -1) {
        freeaddrinfo(addr_p); // remember - cleanup
        return connected_socket;
      }
    }
  }
  freeaddrinfo(addr_p);
  fprintf(stderr,"could not open connection\n");
  return -1;
}

/**
 * @file server.cpp
 * @author Tadeusz Puźniakowski
 * @brief
 * @version 0.1
 * @date 2019-01-16
 *
 * @copyright Copyright (c) 2019 Tadeusz Puźniakowski
 * @license MIT
 */

#include <netdb.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>

/**
 * @brief create listening socket
 *
 * @param success callback that receive bind-ed socket
 * @param error callback on error
 * @param server_name the address on which we should listen
 * @param port_name port on which we listen
 * @param max_queue the number of waiting connections
 */
int listen_server(char *server_name, char *port_name, int max_queue) {
  int listening_socket;
  if (server_name == NULL)
    server_name = "0.0.0.0";
  if (port_name == NULL)
    port_name = "9921";
  if (max_queue <= 0)
    max_queue = 32;
  struct addrinfo hints;
  bzero((char *)&hints, sizeof(struct addrinfo));

  hints.ai_family = AF_UNSPEC;     ///< IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; ///< Stream socket
  hints.ai_flags = AI_PASSIVE;     ///< For wildcard IP address
  hints.ai_protocol = 0;           ///< Any protocol
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  struct addrinfo *result, *rp;
  int s = getaddrinfo(server_name, port_name, &hints, &result);
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
        return listening_socket;
      }
      close(listening_socket); // didn't work, let's close socket
    }
  }
  freeaddrinfo(result);
  fprintf(stderr, "error binding adress\n");
  return -1;
}

/**
 * @brief function that performs accepting of connections.
 *
 * @param listening_socket the correct listening socket
 * @param host if not NULL, then it will get the host name
 * @param ervice if not NULL, then it will get the port (service) name
 *
 * @return connected or failed socket descriptor
 */
int do_accept(int listening_socket, char *host, char *service) {
  struct sockaddr_storage peer_addr;
  socklen_t peer_addr_len = sizeof(struct sockaddr_storage);
  int connected_socket =
      accept(listening_socket, (struct sockaddr *)&peer_addr, &peer_addr_len);

  if (connected_socket == -1) {
    fprintf(stderr, "could not accept connection!\n");
    return connected_socket;
  } else {
    if ((host != NULL) || (service != NULL)) {
      char host_[NI_MAXHOST], service_[NI_MAXSERV];
      getnameinfo((struct sockaddr *)&peer_addr, peer_addr_len, host_,
                  NI_MAXHOST, service_, NI_MAXSERV, NI_NUMERICSERV);
      if (host != NULL)
        strcpy(host, host_);
      if (service != NULL)
        strcpy(service, service_);
    }
    return connected_socket;
  }
}

#ifdef __SERVER
int main(int argc, char **argv) {
  int listening_socket = listen_server("*", (argc > 1) ? argv[1] : "9921", 2);
  printf("Waiting for connection...\n");
  char host[NI_MAXHOST], service[NI_MAXSERV];
  int s = do_accept(listening_socket, host, service);
  char msg[100];
  read(s, msg, 100);
  printf("received \"%s\" from %s connected on %s\n", msg, host, service);
  close(s); // close connected socket
  close(listening_socket); // stop listening on socket
  return 0;
}
#endif

#ifdef __CLIENT
/**
 * @brief The main function. Arguments can be [host] [port] [message to send]
 *
 * It shows how to use the connect_to function
 */

int main(int argc, char **argv) {
  // let's try to connect
  int s = connect_to((argc > 1) ? argv[1] : "localhost",
             (argc > 2) ? argv[2] : "9921");
  if (s > 0) { // success?
    char *msg =
        (argc > 3)
            ? argv[3]
            : "Hi! How are you?"; // we can send also custom messages
    write(s, msg, 100);
    close(s);
    return 0;
  } else {
    return -1;
  }
}
#endif