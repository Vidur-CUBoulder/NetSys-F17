#include "common.h"

int main(int argc, char *argv[])
{
  int sock_fd;
  struct sockaddr_in server_addr;

  
  sock_fd = socket(AF_INET, TCP_SOCKETS, 0);
  //populate_socket_addr(&server_addr, AF_INET, htons(/*atoi(arv[2])*/9001), INADDR_ANY);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(9001);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  
  bind(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));

  listen(sock_fd, 5);

  int client_sock;
  client_sock = accept(sock_fd, NULL, NULL);

  char server_msg[50] = "Hello! My name is Vidur";

  /* Now send the message to the client socket */
  send(client_sock, server_msg, sizeof(server_msg), 0);
  
  close(client_sock);

  return 0;
}



