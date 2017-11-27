#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<stdbool.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>

#define TCP_SOCKETS SOCK_STREAM
#define MAX_CONNECTIONS 100

typedef enum __debug_e {
  FILE_NOT_FOUND = 0,
  NULL_VALUE_RETURNED,
  SUCCESS,
  SUCCESS_RET_STRING,
  BAD_HTTP_REQUEST,
  POST_DATA,
  METHOD_NOT_IMPLEMENTED
} debug_e;

void Create_Server_Connections(int *server_sock, struct sockaddr_in *server_addr,\
                                int server_addr_len, int tcp_port)
{
  if(server_addr == NULL) {
    return;
  }

  (*server_sock) = socket(AF_INET, TCP_SOCKETS, 0);

  /* set the parameters for the server */
  server_addr->sin_family = AF_INET;
  server_addr->sin_port = htons(tcp_port);
  server_addr->sin_addr.s_addr = INADDR_ANY;

  /* Bind the server to the client */
  int ret_val = bind(*server_sock, (struct sockaddr *)server_addr, server_addr_len);
  if(ret_val < 0) {
    perror("ERROR:bind()\n");
    close(*server_sock); 
    exit(1);
  }

  /* Start listening for the connection on the socket 
   * Currently, limit the backlog connections to 5. This value will be 
   * maxed out depending on the value that is in /proc/sys/ipv4/tcp_max_syn_backlog
  */
  ret_val = listen(*server_sock, 5);
  if(ret_val < 0) {
    perror("ERROR:listen()\n");
    exit(1);
  }
  return;
}

int8_t get_client_request(int *accept_socket, int server_socket,\
                        struct sockaddr_in *address, uint32_t addrlen,\
                        char *client_response, uint32_t response_len)
{

  /* Accept any incoming comm. from the client */
  (*accept_socket) = accept(server_socket, (struct sockaddr *)address, &addrlen);
  if(accept_socket < 0) {
    perror("ERROR:accept()\n");
    return -1; 
  }
  
  char filename[100];
  char content_type[20];
  //uint32_t file_size = 0;

  //char *temp_response_str = NULL;
  int8_t fork_pid_val = 0;
  //debug_e debug_ret_val = 0;

  char return_method_string[10];
  memset(return_method_string, '\0', sizeof(return_method_string));

  /* Create another process to handle multiple incoming requests from
   * the web client
   */
  fork_pid_val = fork();
  if(fork_pid_val == 0) {
    
    memset(filename, '\0', sizeof(filename));
    memset(content_type, '\0', sizeof(content_type));
    //file_size = 0;
    
    /* Response to the fork here! Get web client data */
    recv((*accept_socket), client_response, response_len, 0);
    printf("<%s>: client_response: %s\n", __func__, client_response);
    
    /* shutdown the client socket */
    shutdown(*accept_socket, SHUT_RDWR);
    close(*accept_socket);

    /* kill the client process */
    exit(0);
  }

  return 0;
}

