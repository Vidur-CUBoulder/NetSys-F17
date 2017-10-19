#include "common.h"

#define MAX_BUFFER_LENGTH 1024

#define PACKET_COUNT(x) ((x%MAX_BUFFER_LENGTH) == 0) ?\
                      (x/MAX_BUFFER_LENGTH) : ((x/MAX_BUFFER_LENGTH) + 1)

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

  /* Receive some data from the client */
  //printf("<<%s>>\n", __func__);
  recv((*accept_socket), client_response, response_len, 0);
  
  return;
}

void send_file_to_client(char *filename, int accept_socket)
{
  // /home/vidur/NetSys_PAs/Trial_Web_Server/Materials/www 
  char send_buffer[MAX_BUFFER_LENGTH];
  memset(send_buffer, '\0', sizeof(send_buffer));
  uint32_t packet_size = MAX_BUFFER_LENGTH;
  int bytesRead = 0;
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
  //printf("Size of file: %d\n", file_size);
  
  uint32_t file_stream_size = PACKET_COUNT(file_size);
  //printf("file_stream_size: %d\n", file_stream_size);
  
  if(fp != NULL) {  
    while((bytesRead = fread(send_buffer, 1, packet_size, fp)) > 0) {
      
      if(packet_count == (file_stream_size-1)) {
        packet_size = (file_size)%MAX_BUFFER_LENGTH;
      }
      
      send(accept_socket, send_buffer, packet_size, 0);
      packet_count++; 
    }
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
  config_data ws_data;
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
  char http_header[40];
  char content_length[40];
  char content_type[40];
  char data[40];
  char client_response[2048];
  memset(client_response, '\0', sizeof(client_response));

  uint32_t addrlen = sizeof(address);
  while(1)
  {
    memset(client_response, '\0', sizeof(client_response));
    get_client_request(&accept_socket, server_socket, &address,\
        addrlen, client_response, sizeof(client_response));
    printf("<<accept_socket: %d>>\n%s\n", accept_socket, client_response);

    snprintf(http_header, sizeof(http_header), "HTTP/1.1 200 OK\r\n");
    //printf("%s\n", http_header);
    send(accept_socket, http_header, strlen(http_header), 0);

    snprintf(content_length, sizeof(content_length), "Content-length: %d\r\n", 3391);
    //printf("%s\n", content_length);
    send(accept_socket, content_length, strlen(content_length), 0);

    snprintf(content_type, sizeof(content_type), "Content-Type: text/html\r\n\r\n");
    //printf("%s\n", content_type);
    send(accept_socket, content_type, strlen(content_type), 0);

    /* Send the file to the borwser in chunks! */
    send_file_to_client("Materials/www/index.html", accept_socket);
    
    close(accept_socket);
  }

  close(server_socket);
  
  return 0;
}


