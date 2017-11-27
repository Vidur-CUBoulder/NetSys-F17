#include"web_proxy.h"

int32_t client_slots[MAX_CONNECTIONS];

int main(int argc, char *argv[])
{
  /* Get the port number for the proxy server from the 
   * command line
   */
  if(argc > 2 || argc <= 1) {
    printf("Please enter valid command line arguments\n./web_proxy <port_num>\n");
    exit(0);
  }

  uint16_t proxy_portnum = atoi(argv[1]);

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
                    sizeof(address), proxy_portnum);

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
    ret_pid_val = get_client_request(&client_slots[active_connections], server_socket,\
                        &address, addrlen, client_response, sizeof(client_response));
    
    /* Wait for the client process to die */
    waitpid(ret_pid_val, NULL, 0);  
    
    active_connections = (active_connections+1) % 100;
    printf("active_connections: %d\n", active_connections); 
  }
  
  close(server_socket);

  return 0;
}


