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
#include<pthread.h>

#define MAX_CONNECTIONS 100
#define TCP_SOCKETS SOCK_STREAM

typedef struct __user_inputs {
  uint32_t proxy_port_num;
  uint8_t proxy_timeout;
} proxy_user_inputs;

int32_t client_slots[MAX_CONNECTIONS];

void Create_Server_Connections(int *server_sock, struct sockaddr_in *server_addr,\
                                int server_addr_len, int tcp_port)
{
  if(server_addr == NULL) {
    return;
  }

  (*server_sock) = socket(AF_INET, TCP_SOCKETS, 0);
  
  int optval = 1;
  setsockopt((*server_sock), SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
  
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

void *parse_client_request(void *accept_socket_number)
{
  int *accept_socket = (int *)accept_socket_number;

  char client_response[2048];
  memset(client_response, '\0', sizeof(client_response));


  char telnet_string[50];
  int j = 0; int i = 0;
  /* Response to the fork here! Get web client data */
  while(1) {
    memset(telnet_string, '\0', sizeof(telnet_string));  
    recv((*accept_socket), telnet_string, sizeof(telnet_string), 0);
    if(!strcmp(telnet_string, "\r\n")) {
      break;
    }
    j = 0;
    while(j != strlen(telnet_string)) {
      client_response[i++] = telnet_string[j++];
    }
  }

  printf("client_response: %s\n", client_response);
  
  return NULL;
}


int8_t get_client_request(int *accept_socket, int server_socket,\
            struct sockaddr_in *address, uint32_t addrlen)
{

  /* Accept any incoming comm. from the client */
  (*accept_socket) = accept(server_socket, (struct sockaddr *)address, &addrlen);
  if(accept_socket < 0) {
    perror("ERROR:accept()\n");
    return -1; 
  }

  pthread_t thread_pid;
  pthread_create(&thread_pid, NULL, parse_client_request, accept_socket);
  pthread_join(thread_pid, NULL);

  return 0;
}

