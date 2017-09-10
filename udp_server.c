#include "common.h"

//#define SERVER_RESPONSE_ENABLED

char global_server_buffer[2][20];

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
  
  /* call the recvfrom call and block on data from the client */
  int nbytes = 0;
  int packet_count = 0; 
  nbytes = recvfrom(sock_fd, &packet_count, sizeof(packet_count),\
      0, (struct sockaddr *)&remote, &remote_length);
  if(nbytes < 0) {
    perror("ERROR:recvfrom()");
  }
  printf("packet_count: %d\n", packet_count);

  for(int i = 0; i<packet_count; i++) { 
    nbytes = recvfrom(sock_fd, &global_server_buffer[i], sizeof(global_server_buffer),\
        0, (struct sockaddr *)&remote, &remote_length);
    if(nbytes < 0) {
      perror("ERROR:recvfrom()");
    }
  }

#if 0 
  for(int i = 0; i<2; i++) {
    printf("%s\n", global_server_buffer[i]);
  } 
#endif  


#ifdef SERVER_RESPONSE_ENABLED

  /* Send a response back to the client */
  char msg[] = "oranges"; 
  server_response(sock_fd, remote, msg, sizeof(msg));

#endif
  
  close(sock_fd);

  return 0;
}

