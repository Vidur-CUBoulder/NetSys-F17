#include "common.h"

#define MAX_BUFFER_LENGTH 200

#define PACKET_COUNT(x) ((x%MAX_BUFFER_LENGTH) == 0) ?\
                      (x/MAX_BUFFER_LENGTH) : ((x/MAX_BUFFER_LENGTH) + 1)


#define MAX_CONNECTIONS 100

int32_t client_slots[MAX_CONNECTIONS];

/* Some auxiliary functions to offload the work to */
debug_e check_response_command(char *response_str)
{
  if(response_str == NULL) {
    return NULL_VALUE_RETURNED;
  }
  
  /* Check if the method requested is anything other than GET */
  if(!strcmp(response_str, "POST") || !strcmp(response_str, "DELETE") ||\
      !strcmp(response_str, "HEAD") || !strcmp(response_str, "OPTIONS")) {
    /* Send out the appropriate message to the client and exit */
    return METHOD_NOT_IMPLEMENTED;
  }
  
  return SUCCESS;
}

void find_content_type(char *file_extension, char *content_type)
{
  for(int i = 0; i<20; i++) {
    if(!strcmp(file_extension, ws_data.file_index[i])) {
      //printf("ws_data.file_index[i+1]: %s\n", ws_data.file_index[i+1]);
      strcpy(content_type, ws_data.file_index[i+1]);
      return;
    }
  }
  printf("file type not found in index!\n");
  return;
}

debug_e check_http_version(char *version_string)
{
  if(version_string == NULL) {
    return NULL_VALUE_RETURNED;
  }

  int cntr = 0;
  char http_version[5];
  memset(http_version, '\0', sizeof(http_version));
  char *temp = strtok(version_string, "/\0\n");
  
  /* iterate till to get to the end of the string and get the version num */ 
  while(temp != NULL) {
    if(cntr == 1) {
      memcpy(http_version, temp, sizeof(http_version)); 
    }
    cntr++;
    temp = strtok(NULL, "/\n");
  }
  printf("http_version: %s\n", http_version);

  if(!strcmp(http_version, "1.1") || !strcmp(http_version, "1.0"))
    return BAD_HTTP_REQUEST; 
  else 
    return SUCCESS_RET_STRING;
}

debug_e send_file_to_client(char *filename, char *return_string,\
                int accept_socket, uint32_t return_string_len)
{
  char send_buffer[MAX_BUFFER_LENGTH];
  memset(send_buffer, '\0', sizeof(send_buffer));
  uint32_t packet_size = MAX_BUFFER_LENGTH;
  uint32_t bytesRead = 0;
  uint32_t packet_count = 1;
  uint32_t file_size = 0;
  FILE *fp = NULL;
  if (filename != NULL && return_string == NULL) {
    fp = fopen(filename, "rb");
    if(fp == NULL) {
      printf("File open failed!, return!\n");
      return FILE_NOT_FOUND;
    }
    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);
    printf("Size of file: %d\n", file_size);
  } else {
    /* send the return string and return from the function call */
    send(accept_socket, return_string, return_string_len, 0);
    return SUCCESS_RET_STRING;
  }
  
  uint32_t file_stream_size = PACKET_COUNT(file_size);
  printf("file_stream_size: %d\n", file_stream_size);
  
  while((bytesRead = fread(send_buffer, 1, packet_size, fp)) > 0) {

    printf("packet_count: %d; bytesRead: %d\n", packet_count, bytesRead);

    send(accept_socket, send_buffer, bytesRead, 0);
    memset(send_buffer, '\0', sizeof(send_buffer));
    packet_count++; 
  }
  printf("\n");

  return SUCCESS_RET_STRING;
}


void send_http_header(int client_socket, char *header_string, char *content_type,\
                uint32_t content_length, char *connection_type)
{
  if(header_string == NULL || content_type == NULL||\
        connection_type == NULL) {
    printf("Null value passed!\n");
    return;
  }

  char http_header[40];
  snprintf(http_header, sizeof(http_header), "%s\r\n", header_string);
  printf("%s\n", http_header);
  send(client_socket, http_header, strlen(http_header), 0);
    
  char content_type_a[40];
  snprintf(content_type_a, sizeof(content_type_a), "Content-Type: %s\r\n", content_type);
  printf("%s\n", content_type_a);
  send(client_socket, content_type_a, strlen(content_type_a), 0);
    
  char content_length_a[40];
  snprintf(content_length_a, sizeof(content_length_a), "Content-length: %d\r\n",\
                                                    content_length);
  printf("%s\n", content_length_a);
  send(client_socket, content_length_a, strlen(content_length_a), 0);
    
  char connection_a[40];
  snprintf(connection_a, sizeof(connection_a), "Connection: %s\r\n\r\n", connection_type);
  printf("%s\n", connection_a);
  send(client_socket, connection_a, strlen(connection_a), 0);
   
  return;
}


debug_e process_file_response(char *file_request, char *filename,\
                          uint32_t *file_size, char *file_extension, char *content_type)
{

  /* Check if the file exists and if yes, find its size */
  sprintf(filename, "Materials/www%s", file_request);
  printf("filename: %s\n", filename);
  FILE *fp = NULL;
  fp = fopen(filename, "rb");
  if(fp == NULL) {
    printf("File open failed!\n");
    /* This needs to be handled in a different way for errors */
    return FILE_NOT_FOUND; 
  }
  fseek(fp, 0L, SEEK_END);
  *file_size = ftell(fp);
  rewind(fp);
  fclose(fp);

  uint32_t cntr = 0;
  char file_str[50];
  memset(file_str, '\0', sizeof(file_str));
  
  char *temp_buf = strtok(file_request, "/\n");
  while(temp_buf != NULL) {
    if(cntr == 1) {
      memcpy(file_str, temp_buf, sizeof(file_str));
      printf("%s\n", file_str);
    }
    cntr++;
    temp_buf = strtok(NULL, "/\n");
  }
  
  /* If this is just a simple filename like index.html then hack it!! */
  if(file_str[0] == '\0') {
    memcpy(file_str, strtok(file_request, "/\n"), sizeof(file_str));
  }

  char file_chunks[10][10];
  memset(file_chunks, '\0', sizeof(file_chunks));

  /* Parse the file type */
  cntr = 0;
  temp_buf = strtok(file_str, ".\0\n");
  while(temp_buf != NULL) {
    temp_buf = strtok(NULL, ".\0\n");
    if(temp_buf != NULL) {
      memcpy(file_chunks[cntr], temp_buf, sizeof(file_chunks[cntr]));
      cntr++;
    }
  }
  sprintf(file_extension, ".%s", file_chunks[cntr-1]);
  printf("<<%s>>:file_extension: %s\n", __func__, file_extension);
  find_content_type(file_extension, content_type);
  printf("content_type: %s\n", content_type);
  
  return SUCCESS_RET_STRING;

}

void handle_response(int accept_socket, debug_e ret_value,\
                      uint32_t *file_size, char *content_type, char *filename)
{
  debug_e debug_ret_val;

  switch(ret_value)
  {
    case FILE_NOT_FOUND:  /* File not found; return 404 */ 
            printf("In case FILE_NOT_FOUND\n");
            send_http_header(accept_socket, "HTTP/1.1 404 Not Found",\
                    "text/html", strlen(respond_404_error), "close");   
            send_file_to_client(NULL, respond_404_error, accept_socket,\
                                strlen(respond_404_error));
            break;
    
    case SUCCESS:
            printf("In case SUCCESS\n");
            /* Create a function to create the header */
            send_http_header(accept_socket, "HTTP/1.1 200 OK", content_type,\
                                  *file_size, "close");

            /* send the file to the client */
            debug_ret_val = send_file_to_client(filename, NULL, accept_socket, 0);
            break;
    
    case SUCCESS_RET_STRING: /* Do nothing in this case either */
            printf("In case SUCCESS_RET_STRING\n");
            break;
   
    case METHOD_NOT_IMPLEMENTED: /* Send error 501 */
            printf("In case METHOD_NOT_IMPLEMENTED\n");
            send_http_header(accept_socket, "HTTP/1.1 501 Not Implemented",\
                    "text/html", strlen(respond_501_error), "close");
            send_file_to_client(NULL, respond_501_error, accept_socket,\
                                strlen(respond_501_error));
            
            break;
    
    case NULL_VALUE_RETURNED:
            printf("In case NULL_VALUE_RETURNED\n");
            send_http_header(accept_socket, "HTTP/1.1 400 Bad Request",\
                "text/html", strlen(respond_400_error), "close");
            send_file_to_client(NULL, respond_400_error, accept_socket,\
                                strlen(respond_400_error));
            break;
   
    case BAD_HTTP_REQUEST:
            printf("In case BAD_HTTP_REQUEST\n");
            send_http_header(accept_socket, "HTTP/1.1 400 Bad Request",\
                "text/html", strlen(respond_400_error), "close");
            send_file_to_client(NULL, respond_400_error, accept_socket,\
                                strlen(respond_400_error));

            break;

    default:  /* handle the default case */
            printf("in the default case now!\n");
            break;
  }

  return;
}


debug_e parse_client_response(int accept_socket, char *response_str, char *filename,\
                                uint32_t *file_size, char *content_type)
{
  if(response_str == NULL) {
    printf("<%s>:Null Value passed!\n", __func__);
    return NULL_VALUE_RETURNED;
  }
 
  /* Search for and get the correct content type string */
  /* Get the first line only */
  char line_buffer[50];
  memset(line_buffer, '\0', sizeof(line_buffer));
  memcpy(line_buffer, strtok(response_str, "\n"), sizeof(line_buffer));
  printf("line_buffer: %s\n", line_buffer);

  /* Parse elements from the first line as well */
  char line_buffer_words[3][50];
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
  cntr++; //net num of words in the string.
  printf("line_buffer_words[0]: %s\n", line_buffer_words[0]);
  printf("line_buffer_words[1]: %s\n", line_buffer_words[1]);
  printf("line_buffer_words[2]: %s\n", line_buffer_words[2]);

  debug_e ret_val = check_response_command(line_buffer_words[0]);
  if(ret_val != SUCCESS)
    return ret_val;

  char file_extension[10];
  memset(file_extension, '\0', sizeof(file_extension));
  ret_val = process_file_response(line_buffer_words[1], filename, file_size,\
                                      file_extension, content_type);
  if(ret_val != SUCCESS_RET_STRING)
    return ret_val;
  
  ret_val = check_http_version(line_buffer_words[2]);
  if(ret_val != SUCCESS_RET_STRING)
    return ret_val;
 
  return SUCCESS;
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
  
  int8_t fork_pid_val = 0;
  debug_e debug_ret_val = 0;

  /* Create another process to handle multiple incoming requests from
   * the web client
   */
  fork_pid_val = fork();
  if(fork_pid_val == 0) {
    
    memset(filename, '\0', sizeof(filename));
    memset(content_type, '\0', sizeof(content_type));
    file_size = 0;
    
    /* Response to the fork here! Get web client data */
    recv((*accept_socket), client_response, response_len, 0);

    /* Parse the response from the client */
    debug_ret_val = parse_client_response(*accept_socket, client_response, filename,\
                          &file_size, content_type);
    handle_response(*accept_socket, debug_ret_val, &file_size, content_type,\
                                      filename);
    
    /* shutdown the client socket */
    shutdown(*accept_socket, SHUT_RDWR);
    close(*accept_socket);

    /* kill the client process */
    exit(0);
  }

  return fork_pid_val;
}

int main(int argc, char *argv[])
{
  /* First order of business is to open the .conf file, parse it 
   * and store its contents in data structures. All of these will
   * be used later.
   */
  WS_Parse_Config_File(&ws_data); 
  
  int server_socket = 0;

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(server_socket < 0) {
    perror("ERROR:socket()\n");
    return 0;
  }
  
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
 
  /* Setup the webserver connections */
  Create_Server_Connections(&server_socket, &address,\
                    sizeof(address), ws_data.port);

  int active_connections = 0;

  char client_response[2048];
  memset(client_response, '\0', sizeof(client_response));

  uint32_t addrlen = sizeof(address);
  int8_t ret_pid_val = 0;

  while(1)
  {
    /* Clear the client_response buffer and get fresh data from the 
     * client. Act as per the data received and sent data to the client
     */
    ret_pid_val = 0;
    memset(client_response, '\0', sizeof(client_response));
    
    /* Get the update data form the client and store in client_response buffer */
    ret_pid_val = get_client_request(&client_slots[active_connections], server_socket, &address,\
                                  addrlen, client_response, sizeof(client_response));
    printf("<<client_slots: %d>>\n%s\n", client_slots[active_connections], client_response);
    
    /* Wait for the client process to die */
    waitpid(ret_pid_val, NULL, 0);  
    
    active_connections = (active_connections+1) % 100;
  }

  close(server_socket);
  
  return 0;
}


