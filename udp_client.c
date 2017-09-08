#include "common.h"

#define CLIENT_RESPONSE_ENABLED

int client_response(int sock_fd, struct sockaddr_in remote_socket, \
                  char *response, int response_size)
{
  /* Block till response from server received ! */ 
  socklen_t addr_len = sizeof(struct sockaddr);
  int nbytes = recvfrom(sock_fd, response, response_size,0,\
      (struct sockaddr *)&remote_socket, &addr_len);    
  if(nbytes < 0) {
    perror("client:RECVFROM()");
    return nbytes;
  }
  
  return nbytes;
}


int main(int argc, char *argv[])
{
  int sock_fd = 0;  
  struct sockaddr_in remote;

  /* check the arguments passed from the command line */
  if(argc < 3) {
    printf("USAGE: <server ip> <server port>\n");
    exit(1);
  }
  
  sock_fd = populate_socket_addr(&remote, AF_INET, htons(atoi(argv[2])),\
                        (inet_addr(argv[1])));

  /* Create a buffer from which to send the data */
  char client_data_buffer[MAX_BUFFER_LENGTH] = "apple";

  /* Send the data to the server */
  int nbytes = sendto(sock_fd, &client_data_buffer, sizeof(client_data_buffer),\
      0, (struct sockaddr *)&remote, (socklen_t)sizeof(remote));
  if(nbytes < 0) {
    perror("ERROR: sendto()");
    exit(1);
  }

  printf("Data successfully sent from the client to the server!\n");

#ifdef CLIENT_RESPONSE_ENABLED
 
  /* Block till response from server received ! */ 
  char msg[MAX_BUFFER_LENGTH];
  client_response(sock_fd, remote, msg, sizeof(msg));
  printf("return data from the server: %s\n", msg);

#endif
  
  close(sock_fd);

  return 0;
}
