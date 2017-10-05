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
#include<inttypes.h>

#define TCP_SOCKETS SOCK_STREAM

#define DEBUG_CONNECTIONS

/* Let's create a list/array of the keywords that we are searching for */
#if 0
char *ws_conf_keywords[] = {   "Listen",\
                               "DocumentRoot",\
                               "DirectoryIndex",\
                               "."
                            };
#endif

typedef struct ws_config_data {
  size_t port;
  char directory_root[50];
  char default_html[20];
  char file_index[20][30];
} config_data;

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
  bind(*server_sock, (struct sockaddr *)server_addr, server_addr_len);

  /* Start listening for the connection on the socket 
   * Currently, limit the backlog connections to 5. This value will be 
   * maxed out depending on the value that is in /proc/sys/ipv4/tcp_max_syn_backlog
  */
  listen(*server_sock, 5);

  return;
}



