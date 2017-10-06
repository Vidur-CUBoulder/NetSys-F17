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

enum http_version_numbers {
  HTTP_VERSION_1_0 = 0, 
  HTTP_VERSION_1_1 = 1
};

char *HTTP_Versions[] = {"1.0", "1.1"};


typedef struct ws_config_data {
  size_t port;
  char directory_root[50];
  char default_html[20];
  char file_index[20][30];
} config_data;

typedef struct http_header_struct {
  char http_version[3]; 
  
  uint32_t status_code;
  
  union __reply_code {
    char reply_ok[3];
    char reply_doc_follows[18];
  } u_reply_code;
  
  size_t file_content_length;
  char file_content_type[10];
} webserver_http_header;

void Create_Header(char *header_string, webserver_http_header *header, int version_num,\
                                    uint32_t status, size_t content_length, char *content_type)
{
  char status_code_string[10];
  char content_length_string[10];
  /* initialize the HTTP header witht the default strings and values */
  if(version_num) {
    strcpy(header->http_version, HTTP_Versions[1]);
  } else {
    strcpy(header->http_version, HTTP_Versions[0]);
  }

  /* Store the code and convert to string */
  header->status_code = status;
  sprintf(status_code_string, "%d", header->status_code);

  strcpy(header->u_reply_code.reply_ok, "OK");
  strcpy(header->u_reply_code.reply_doc_follows, "Document Follows");

  /* Store the content-length string and content-type string and convert to string type */
  header->file_content_length = content_length;
  snprintf(content_length_string, sizeof(content_length_string), "%lu",\
                    header->file_content_length);
  strcpy(header->file_content_type, content_type);

  /* Create the string and pass it back to the calling function */
  snprintf(header_string, 300, "%s%s %s %s\nContent-Length: %s\nContent-Type: %s\n\n",\
          "HTTP/", header->http_version, status_code_string,\
          header->u_reply_code.reply_ok, content_length_string,\
                    header->file_content_type);
  
  printf("%s", header_string);

  return;
}


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



