#include"webproxy.h"

int main(int argc, char *argv[])
{
  /* Get the port number for the proxy server from the 
   * command line
   */
  if(argc > 3 || argc <= 1) {
    printf("Please enter valid command line arguments\n./web_proxy <port_num> <timeout period>\n");
    exit(0);
  }

  proxy_user_inputs user_configs;
  user_configs.proxy_port_num = atoi(argv[1]);
  user_configs.proxy_timeout = atoi(argv[2]);

  /* initialize all socket values */
  memset(client_slots, -1, sizeof(client_slots));

  int server_socket = 0;
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
 
  /* Setup the webserver connections */
  Create_Server_Connections(&server_socket, &address,\
            sizeof(address), user_configs.proxy_port_num);

  size_t addrlen = sizeof(address);
  int ret_val = 0;
  int16_t active_connections = 0;
  
  while(1) {
    
    ret_val = get_client_request(&client_slots[active_connections], server_socket,\
                              &address, addrlen);
    if(ret_val < 0) {
      printf("Socket not accepted\n");
      return 0;
    }
    
    active_connections = (active_connections+1) % 100;
    printf("active_connections: %d\n", active_connections); 
  }

  close(server_socket);

  return 0;
}

