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

#define COMMAND_WORD_INDEX 0
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

typedef struct __URL_information {
  uint32_t port_num;
  char ip_addr[20];
  char domain_name[60];
  char url_path[200];
  char http_version[20];
  char http_msg_body[200];
} url_info;


void Create_Server_Connections(int *server_sock, struct sockaddr_in *server_addr,\
                                int server_addr_len, int tcp_port)
{
  if(server_addr == NULL) {
    return;
  }

  (*server_sock) = socket(AF_INET, TCP_SOCKETS, 0);
  //(*server_sock) = socket(AF_INET, TCP_SOCKETS, SO_REUSEADDR);
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

debug_e check_response_command(char *response_str)
{
  if(response_str == NULL) {
    return NULL_VALUE_RETURNED;
  }

  if(!strcmp(response_str, "GET"))
    return SUCCESS;
  else
    return METHOD_NOT_IMPLEMENTED;
}

int hostname_to_ip(char * hostname , char* ip)
{
  struct hostent *he;
  struct in_addr **addr_list;
  int i;

  if ( (he = gethostbyname( hostname ) ) == NULL) 
  {
    // get the host info
    herror("gethostbyname");
    return 1;
  }

  addr_list = (struct in_addr **) he->h_addr_list;

  for(i = 0; addr_list[i] != NULL; i++) 
  {
    //Return the first one;
    strcpy(ip , inet_ntoa(*addr_list[i]) );
    return 0;
  }

  return 1;
}

#if 0
debug_e parse_client_response(int accept_socket, char *response_str, char *filename,\
    uint32_t *file_size, char *content_type, char *return_method_string, url_info *url)
{
  
  if(response_str == NULL) {
    return NULL_VALUE_RETURNED;
  }

  char *temp_response_str = response_str;
  
  /* Search for and get the correct content type string */
  /* Get the first line only */
  char line_buffer[500];
  memset(line_buffer, '\0', sizeof(line_buffer));
  memcpy(line_buffer, strtok(temp_response_str, "\n"), sizeof(line_buffer));

  printf("<%s>:line_buffer: %s\n", __func__, line_buffer);
  
  /* Parse elements from the first line as well */
  char line_buffer_words[5][500];
  uint8_t cntr = 0;
  char *temp_buf = NULL;
  memset(line_buffer_words, '\0', sizeof(line_buffer_words));
  
  temp_buf = strtok(line_buffer, " \n");
  memcpy(line_buffer_words[cntr], temp_buf, sizeof(line_buffer_words[cntr]));
  
  while(temp_buf != NULL) {
    temp_buf = strtok(NULL, " \n");
    if(temp_buf != NULL) {
      cntr++;
      memcpy(line_buffer_words[cntr], temp_buf, sizeof(line_buffer_words[cntr]));
    }
  }
  
  cntr++; /*net num of words in the string.*/
  debug_e ret_val = check_response_command(line_buffer_words[COMMAND_WORD_INDEX]);
  if(ret_val != SUCCESS) {
    memcpy(return_method_string, line_buffer_words[COMMAND_WORD_INDEX],\
                      sizeof(line_buffer_words[COMMAND_WORD_INDEX]));
    printf("Exiting; invalid command!\n");
    return ret_val;
  }

#if 0
  printf("<%s>: line_buffer_words: %s\n", __func__,\
                    line_buffer_words[COMMAND_WORD_INDEX]);
  printf("<%s>: line_buffer_words: %s\n", __func__, line_buffer_words[1]);
  printf("<%s>: line_buffer_words: %s\n", __func__, line_buffer_words[2]);
#endif

  char temp_buf_2[400];
  memset(temp_buf_2, '\0', sizeof(temp_buf_2));
  memcpy(temp_buf_2, line_buffer_words[1], strlen(line_buffer_words[1]));
  
  char *remaining_buf = NULL;
  /* Parse the URL to get the IP and the port number */
  temp_buf = line_buffer_words[1];
  memcpy(url->ip_addr, strtok(temp_buf, ":"), sizeof(url->ip_addr));
  
  if(!strcmp(url->ip_addr, "127.0.0.1")) {
    /* Continue parsing the string */
    while(temp_buf != NULL) {
      temp_buf = strtok(NULL, "\n");
      if(temp_buf == NULL)
        break;
      remaining_buf = temp_buf;
    }

    url->port_num = atoi(strtok(remaining_buf, "/\n"));
    while(remaining_buf != NULL) {
      remaining_buf = strtok(NULL, "\n");
      if(remaining_buf == NULL)
        break;
      memcpy(url->url_path, remaining_buf, strlen(remaining_buf));
    }
  } else {
    /* We need to resolve the hostname and get the IP address
     * The port number in this case is default to 80
     */
    printf("temp_buf_2: %s\n", temp_buf_2);
    
    /* Set to default port number */
    memcpy(url->url_path, temp_buf_2, strlen(temp_buf_2));
    url->port_num = 80;
    char *temp_url_path = url->url_path;
    strtok(temp_url_path, "/\n");
    memcpy(url->domain_name, temp_url_path, strlen(temp_url_path)); 
    //printf("%s\n", temp_url_path);
    temp_url_path = strtok(NULL, "\n");
    memcpy(url->url_path, temp_url_path, strlen(temp_url_path));
    //printf("%s\n", temp_url_path);
    /*while(temp_url_path != NULL) {
      printf("%s\n", temp_url_path);
      temp_url_path = strtok(NULL, "/\n");
    }*/
  }
 
  /* store the HTTP version tag */
  memcpy(url->http_version, line_buffer_words[2], strlen(line_buffer_words[2]));

  hostname_to_ip(url->domain_name, url->ip_addr); 
  
  /* Store the message body if any */
  if(line_buffer_words[3] != NULL) {
    memcpy(url->http_msg_body, line_buffer_words[3], strlen(line_buffer_words[3]));
  }

  printf("url->ip_addr: %s\n",  url->ip_addr);
  printf("url->port_num: %d\n", url->port_num);
  printf("url->domain_name: %s\n", url->domain_name);
  printf("url->url_path: %s\n", url->url_path);
  printf("url->http_version:  %s\n", url->http_version);
  printf("url->http_msg_body: %s\n", url->http_msg_body);

  return SUCCESS;
}
#endif

void parse_client_response(int accept_socket, char *response_str, url_info *url)
{
  printf("<%lu>:response_str: %s\n", strlen(response_str), response_str);
  char line_buffer_words[5][500];
  memset(line_buffer_words, '\0', sizeof(line_buffer_words));

  uint8_t word_cntr = 0;
  char temp_word_buffer[5][500];
  memset(temp_word_buffer, '\0', sizeof(temp_word_buffer));

  uint8_t line_cntr = 0;
  char *temp_buf = strtok(response_str, "\n");
  while(temp_buf != NULL) {
    memcpy(line_buffer_words[line_cntr++], temp_buf, strlen(temp_buf));
    temp_buf = strtok(NULL, "\n");
  }

  /* Parse the first line of the GET request */
  temp_buf = NULL;
  char *temp_buf_1 = NULL;
  temp_buf_1 = line_buffer_words[0];
  temp_buf = strtok(temp_buf_1, " \n");
  while(temp_buf != NULL) {
    memcpy(temp_word_buffer[word_cntr++], temp_buf, strlen(temp_buf)); 
    temp_buf = strtok(NULL, " \n");
  }

  /* Check for the type of request and return on failure */
  if(strcmp(temp_word_buffer[0], "GET")) {
    /* Request type is not GET */
    printf("Req. type is NOT GET!\n");
    return;
  }
  
  /* Set to the default port number */
  url->port_num = 80;

  /* Store the URL path */
  memcpy(url->url_path, temp_word_buffer[1], strlen(temp_word_buffer[1]));

  /* Store the http request version */
  memcpy(url->http_version, temp_word_buffer[2], strlen(temp_word_buffer[2]));

  temp_buf = NULL;
  temp_buf_1 = NULL;
  memset(temp_word_buffer, '\0', sizeof(temp_word_buffer));
  word_cntr = 0;
  temp_buf_1 = line_buffer_words[1];

  temp_buf = strtok(temp_buf_1, ":\n");
  while(temp_buf != NULL) {
    memcpy(temp_word_buffer[word_cntr++], temp_buf, strlen(temp_buf)); 
    temp_buf = strtok(NULL, " \n");
  }

  /* Set the Hostname from the request */
  memcpy(url->domain_name, temp_word_buffer[1], (strlen(temp_word_buffer[1])-1));

  /* Get the IP address for the domain name and store it in the struct */
  hostname_to_ip("www.google.com", url->ip_addr); 

  printf("url->ip_addr: %s\n",  url->ip_addr);
  printf("url->port_num: %d\n", url->port_num);
  printf("<%lu>: url->domain_name: %s\n", strlen(url->domain_name), url->domain_name);
  printf("url->url_path: %s\n", url->url_path);
  printf("url->http_version:  %s\n", url->http_version);
  //printf("url->http_msg_body: %s\n", url->http_msg_body);


  return;
}

void get_server_response(url_info *url)
{
  /* Create a scoket connection to the internet */
  struct sockaddr_in addr;
  bzero((char *)&addr, sizeof(addr));
  
  struct hostent *server = NULL;
  server = gethostbyname(url->ip_addr);
  if(server == NULL) {
    printf("Error: No such host!\n");
    return;
  }

  int sock = 0;
  sock = socket(AF_INET, SOCK_STREAM, 0);

  addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&addr.sin_addr.s_addr, server->h_length);
  addr.sin_port = htons(80);
  
  
  if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
    perror("Connect");
    return;
  }

  char buffer[4096];
  char request[80];
  memset(request, '\0', sizeof(request));
  sprintf(request, "GET %s %s\r\nConnection: close\r\n\r\n",\
                           url->url_path, url->http_version);
  printf("<%s>:web request: %s\n", __func__, request);

  int n = write(sock, request, strlen(request));
  if(n < 0) 
    printf("Error: writing to socket\n");

  while(1) {
    bzero(buffer, 4096);
    n = recv(sock, buffer, 4095, 0);
    if(n < 0) {
      printf("Error reading from the socket!\n");
      break;
    }
    if(n == 0) {
      break;
    }

    //n = read(sock, buffer, 4095);

    printf("%d\n", (int)strlen(buffer));
    printf("%s\n", buffer);
  }
  
  close(sock);

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
  uint32_t file_size = 0;

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
    file_size = 0;
    
    char telnet_string[50];
    int j = 0; int i = 0;
    /* Response to the fork here! Get web client data */
    while(1) {
      memset(telnet_string, '\0', sizeof(telnet_string));  
      recv((*accept_socket), telnet_string, sizeof(telnet_string), 0);
      if(!strcmp(telnet_string, "\r\n")) {
        printf("end!\n");
        break;
      }
      j = 0;
      while(j != strlen(telnet_string)) {
        client_response[i++] = telnet_string[j++];
      }
    }
    //printf("client_response: %s\n", client_response);

    url_info url;
    memset(&url.port_num, 0, sizeof(url.port_num));
    memset(url.ip_addr, '\0', sizeof(url.ip_addr));
    memset(url.url_path, '\0', sizeof(url.url_path));
    memset(url.http_version, '\0', sizeof(url.http_version));
    memset(url.http_msg_body, '\0', sizeof(url.http_msg_body));
    memset(url.domain_name, '\0', sizeof(url.domain_name));

    /* Parse the response from the client */
    /*parse_client_response((*accept_socket), client_response, filename, &file_size,\
                        content_type, return_method_string, &url);*/
    parse_client_response((*accept_socket), client_response, &url);

    get_server_response(&url);
    /* shutdown the client socket */
    shutdown(*accept_socket, SHUT_RDWR);
    close(*accept_socket);

    /* kill the client process */
    exit(0);
  }

  return 0;
}





