#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define MAX_BUFFER_LENGTH 100

#define UDP_SOCKETS SOCK_DGRAM

int populate_socket_addr(struct sockaddr_in *sock, uint8_t sock_family,\
                            uint16_t sock_port, in_addr_t sock_addr)
{

  bzero(sock, sizeof(struct sockaddr_in)); 
  sock->sin_family = sock_family;
  sock->sin_port = sock_port;
  sock->sin_addr.s_addr = sock_addr;

  int sock_fd = socket(sock_family, UDP_SOCKETS, 0);
  if(sock_fd < 0) {
    perror("ERROR:socket()");
    exit(1);
  }
  return sock_fd;
}




