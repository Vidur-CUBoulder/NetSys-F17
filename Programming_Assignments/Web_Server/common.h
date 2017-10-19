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
  int ret_val = bind(*server_sock, (struct sockaddr *)server_addr, server_addr_len);
  if(ret_val < 0) {
    perror("ERROR:bind()\n");
    return;
  }

  /* Start listening for the connection on the socket 
   * Currently, limit the backlog connections to 5. This value will be 
   * maxed out depending on the value that is in /proc/sys/ipv4/tcp_max_syn_backlog
  */
  ret_val = listen(*server_sock, 5);
  if(ret_val < 0) {
    perror("ERROR:listen()\n");
    return;
  }
  return;
}

void WS_Parse_Config_File(config_data *ws_data)
{
  FILE *fp = NULL;
  ssize_t read_line = 0;
  char *line = (char *)malloc(50);
  int cntr_words = 0;
  char *eptr;

  fp = fopen("./Materials/ws.conf", "r");
  if(fp == NULL) {
    /* Unable to open the file */
    exit(EXIT_FAILURE);
  }
  
  while( (read_line = getline(&line, &read_line, fp)) != -1) 
  {
    /* Ignore statements starting with '#' */
    if(line[0] == '#')
      continue;
   
    char *buffer = strtok(line, " \n"); 
    while(buffer != NULL) 
    {
      switch(cntr_words)
      {
        case 1: /* Store the Port number */
                ws_data->port = strtol(buffer, &eptr, 10);
                //printf("cntr_words: %d; port: %lu\n", cntr_words, ws_data->port);
                break;

        case 3: /* Store the directory root */
                strcpy(ws_data->directory_root,buffer); 
                //printf("%s\n", ws_data->directory_root);
                break;

        case 5: /* Store the default index file name */
                strcpy(ws_data->default_html, buffer);
                //printf("%s\n", ws_data->default_html);
                break;

        default:
                break;
      }
     
      if(cntr_words >= 6) {
        strcpy(ws_data->file_index[cntr_words-6], buffer);
        //printf("%s\n", ws_data->file_index[cntr_words-6]);
      }
      
      buffer = strtok(NULL, " \n");
      cntr_words++;
    }
  }
  
  fclose(fp);
  free(line);

  return;
}


