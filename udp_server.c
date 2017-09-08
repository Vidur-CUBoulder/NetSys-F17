#include "common.h"

#define SERVER_RESPONSE_ENABLED

int server_response(int sock_fd, struct sockaddr_in remote_socket, \
            char *response, int response_size)
{
  int nbytes = sendto(sock_fd, response, response_size, 0,\
                (struct sockaddr *)&remote_socket,\
                (socklen_t)sizeof(struct sockaddr_in));
  if(nbytes < 0) {
    perror("server:SENDTO");
    return nbytes;
  }

  return nbytes;
}


int main (int argc, char * argv[] )
{

  int sock_fd = 0;
  struct sockaddr_in server_socket, remote;

  /* check if arguments were passed from the command line interface */
  if(argc != 2) {
    printf("Usage: <port number>\n");
    exit(1);
  }
 
  sock_fd = populate_socket_addr(&server_socket, AF_INET, htons(atoi(argv[1])),\
                          INADDR_ANY);

  /* Bind the created socket to the provided address and port number */
  int bind_ret = bind(sock_fd, (struct sockaddr *)&server_socket,\
                      sizeof(server_socket));
  if(bind_ret < 0) {
    perror("ERROR:bind()");
    exit(1);
  }

  /* Create a buffer to store the incoming data from the client in */
  unsigned int remote_length = sizeof(remote);
  char server_data_buffer[MAX_BUFFER_LENGTH];
  bzero(server_data_buffer, sizeof(server_data_buffer));
  
  /* call the recvfrom call and block on data from the client */
  int nbytes = recvfrom(sock_fd, &server_data_buffer, sizeof(server_data_buffer),\
      0, (struct sockaddr *)&remote, &remote_length);
  if(nbytes < 0) {
    perror("ERROR:recvfrom()");
  }

  printf("Data from client: %s\n", server_data_buffer);

#ifdef SERVER_RESPONSE_ENABLED

  /* Send a response back to the client */
  char msg[] = "oranges"; 
  server_response(sock_fd, remote, msg, sizeof(msg));

#endif
  
  close(sock_fd);

  return 0;
}

