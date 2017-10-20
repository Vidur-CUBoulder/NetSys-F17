#include "common.h"

#define MAX_BUFFER_LENGTH 200

#define PACKET_COUNT(x) ((x%MAX_BUFFER_LENGTH) == 0) ?\
                      (x/MAX_BUFFER_LENGTH) : ((x/MAX_BUFFER_LENGTH) + 1)


int32_t client_slots[100];

/* Open the socket connection to the client and get the 
 * initial client request for the file
 */
void get_client_request(int *accept_socket, int server_socket,\
                        struct sockaddr_in *address, uint32_t addrlen,\
                        char *client_response, uint32_t response_len)
{

  /* Accept any incoming comm. from the client */
  (*accept_socket) = accept(server_socket, (struct sockaddr *)address, &addrlen);
  if(accept_socket < 0) {
    perror("ERROR:accept()\n");
    return; 
  }
  //client_slots[cntr] = (*accept_socket);
  
  //if(fork() == 0) {
    /* Respond to the request */
    //exit(0);
  //}
  //cntr++;
  /* Receive some data from the client */
  //printf("<<%s>>\n", __func__);
  recv((*accept_socket), client_response, response_len, 0);
  
  return;
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

void parse_client_response(char *response_str, char *filename, uint32_t *file_size,\
                            char *content_type)
{
  if(response_str == NULL) {
    printf("<%s>:Null Value passed!\n", __func__);
    return;
  }

  char *buffer = strtok(response_str, "GET "); 

  sprintf(filename, "Materials/www%s", buffer);
  printf("filename: %s\n", filename);
  FILE *fp = NULL;
  fp = fopen(filename, "rb");
  if(fp == NULL) {
    printf("File open failed!\n");
    /* This needs to be handled in a different way for errors */
    return;
  }
  fseek(fp, 0L, SEEK_END);
  *file_size = ftell(fp);
  rewind(fp);
  fclose(fp);

  /* Search for and get the correct content type string */
  /* Get the first line only */
  char line_buffer[50];
  memset(line_buffer, '\0', sizeof(line_buffer));
  memcpy(line_buffer, strtok(response_str, "\n"), sizeof(line_buffer));
  printf("%s\n", line_buffer);

  /* Parse elements from the first line as well */
  char line_buffer_words[5][50];
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
  printf("line_buffer_words[1]: %s\n", line_buffer_words[1]);


  cntr = 0;
  char file_str[50];
  memset(file_str, '\0', sizeof(file_str));
  temp_buf = strtok(line_buffer_words[1], "/\n");
  while(temp_buf != NULL) {
    if(cntr == 1) {
      memcpy(file_str, temp_buf, sizeof(file_str));
      printf("%s\n", file_str);
    }
    cntr++;
    temp_buf = strtok(NULL, "/\n");
  }
  
  if(file_str[0] == '\0') {
    memcpy(file_str, strtok(line_buffer_words[1], "/\n"), sizeof(file_str));
  }

  char file_chunks[10][10];
  memset(file_chunks, '\0', sizeof(file_chunks));
  char file_extension[10];
  memset(file_extension, '\0', sizeof(file_extension));

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
  //printf("cntr: %d\n", cntr);
  //printf("file_chunks[cntr]: %s\n", file_chunks[cntr-1]);

  return;
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

void send_file_to_client(char *filename, int accept_socket)
{
  char send_buffer[MAX_BUFFER_LENGTH];
  memset(send_buffer, '\0', sizeof(send_buffer));
  uint32_t packet_size = MAX_BUFFER_LENGTH;
  uint32_t bytesRead = 0;
  uint32_t packet_count = 1;

  FILE *fp = NULL;
  fp = fopen(filename, "rb");
  if(fp == NULL) {
    printf("File open failed!, return!\n");
    exit(1);
    return;
  }
  fseek(fp, 0L, SEEK_END);
  uint32_t file_size = ftell(fp);
  rewind(fp);
  printf("Size of file: %d\n", file_size);
  
  uint32_t file_stream_size = PACKET_COUNT(file_size);
  printf("file_stream_size: %d\n", file_stream_size);
  
  while((bytesRead = fread(send_buffer, 1, packet_size, fp)) > 0) {

    printf("packet_count: %d; bytesRead: %d\n", packet_count, bytesRead);

    if(packet_count == (file_stream_size-1)) {
      packet_size = (file_size)%MAX_BUFFER_LENGTH;
    }

    send(accept_socket, send_buffer, packet_size, 0);
    memset(send_buffer, '\0', sizeof(send_buffer));
    packet_count++; 
  }
  printf("\n");

  return;
}

int main(int argc, char *argv[])
{
  /* First order of business is to open the .conf file, parse it 
   * and store its contents in data structures. All of these will
   * be used later.
   */
  WS_Parse_Config_File(&ws_data); 
  
  int server_socket = 0;
  //int accept_socket = 0;

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

  int accept_socket = 0;
  char client_response[2048];
  char content_type[20];
  char filename[100];
  uint32_t file_size = 0;
  memset(client_response, '\0', sizeof(client_response));
  memset(filename, '\0', sizeof(filename));
  memset(content_type, '\0', sizeof(content_type));

  uint32_t addrlen = sizeof(address);
  while(1)
  {
    /* Clear the client_response buffer and get fresh data from the 
     * client. Act as per the data received and sent data to the client
     */
    memset(client_response, '\0', sizeof(client_response));
    
    /* Get the update data form the client and store in client_response buffer */
    get_client_request(&accept_socket, server_socket, &address,\
        addrlen, client_response, sizeof(client_response));
    printf("<<accept_socket: %d>>\n%s\n", accept_socket, client_response);
   
    /* Parse the client response */
    parse_client_response(client_response, filename, &file_size, content_type);
      
    /* Create a function to create the header */
    send_http_header(accept_socket, "HTTP/1.1 200 OK", content_type, file_size, "close");
    
    send_file_to_client(filename, accept_socket);
    
    close(accept_socket);
    
  }

  close(server_socket);
  
  return 0;
}


