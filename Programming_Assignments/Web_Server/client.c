#include "common.h"

int main(int argc, char *argv[])
{

  struct sockaddr_in server_addr;

  int sock_fd = socket(AF_INET, TCP_SOCKETS, 0);
  //populate_socket_addr(&server_addr, AF_INET, htons(/*atoi(arv[2])*/9001), INADDR_ANY);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(9001);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  int ret_val = connect(sock_fd,(struct sockaddr *)&server_addr, sizeof(server_addr));
  if(ret_val < 0) {
    perror("ERROR: CONNECT()\n");
    close(sock_fd);
  }

  char store_string[50];
  recv(sock_fd, store_string, sizeof(store_string), 0);

  printf("Data recv:\n");
  printf("%s\n", store_string);

  close(sock_fd);

  return 0;
}


