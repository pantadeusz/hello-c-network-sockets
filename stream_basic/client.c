/**
 * @author Tadeusz Puźniakowski
 * @copyright Copyright (c) 2019 Tadeusz Puźniakowski
 * @license MIT
 */


#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  struct addrinfo hints;
  bzero((char *)&hints, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;     ///< IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; ///< stream socket

  struct addrinfo *addr_p;
  if (argc < 4) {
    fprintf(stderr,"Running:\n   %s [host] [port] [message]\n",argv[0]);
  }
  int err = getaddrinfo(argv[1], argv[2], &hints, &addr_p);
  if (err) {
    fprintf(stderr, gai_strerror(err));
    return -1;
  }
  struct addrinfo *rp;
  // find first working address that we can connect to
  for (rp = addr_p; rp != NULL; rp = rp->ai_next) {
    int connected_socket =
        socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (connected_socket != -1) {
      if (connect(connected_socket, rp->ai_addr, rp->ai_addrlen) != -1) {
        // we managed to connect!!! Send some data!
        int arglen = strlen(argv[3]);
        
        write(connected_socket, &arglen,sizeof(arglen));
        write(connected_socket, argv[3], arglen);
        close(connected_socket);

        freeaddrinfo(addr_p); // remember - cleanup
        return 0;             // connected_socket;
      }
    }
  }
  freeaddrinfo(addr_p);
  fprintf(stderr, "could not open connection\n");
  return -1;
}
