#include "common.h"

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


int main(int argc, char *argv[])
{
  /* First order of business is to open the .conf file, parse it 
   * and store its contents in data structures. All of these will
   * be used later.
   */
  config_data ws_data;
  WS_Parse_Config_File(&ws_data); 
 
  /* Create the header string to send to the client */
  char http_header_string[2048];
  webserver_http_header http_header;
 
#ifdef DEBUG_CONNECTIONS
  char response_data[1024] = "Hello There!! WebBrowser.This is Pathetic!!Adding more data!!";
#endif
  Create_Header(http_header_string, &http_header, HTTP_VERSION_1_1, 200, strlen(response_data), "text/plain");
  strncat(http_header_string, response_data, strlen(response_data));
  
  char client_response[2048];

  /* Now, create the TCP connection to the client(browser) */
  int server_sock = 0;
  struct sockaddr_in server_addr;
  memset(&server_sock, 0, sizeof(server_addr));
  
  /* Setup the webserver connections */
  Create_Server_Connections(&server_sock, &server_addr,\
                    sizeof(server_addr), ws_data.port);

  /* Provision for client connections */
  int client_sock = 0;
  while(1) {
    client_sock = accept(server_sock, NULL, NULL);
    
    /* Send data to the client */
    send(client_sock, http_header_string, sizeof(http_header_string), 0); 

    /* Receive some data from the client */
    recv(client_sock, client_response, sizeof(client_response), 0);
    printf("%s\n", client_response);

    /* Close the client socket connection once you are done */
    close(client_sock);
  }
  return 0;
}


