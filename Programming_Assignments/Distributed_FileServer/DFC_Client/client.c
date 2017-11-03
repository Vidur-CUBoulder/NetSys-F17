#include"../common/common.h"

int main(int argc, char *argv[])
{
  /* Get the input from the command line */
  if(argc == 1) {
    printf("Configuration file not passed!\n");
    exit(0);
  }

  /* Parse the client config file and store the data in structures */
  client_config_data_t client_data;
  Parse_Client_Config_File(argv[1], &client_data);
  Dump_Client_Data(&client_data, 4);
 
  /* Make a connection to the 4 servers */
  uint8_t client_socket[4];
  memset(client_socket, '\0', sizeof(client_socket));

  struct sockaddr_in server_addr[4];
  memset(&server_addr, 0, sizeof(server_addr));

  for(int i = 0; i<4; i++) {
    Create_Client_Connections(&client_socket[i], client_data.client_ports.port_num[i],\
        &server_addr[i], sizeof(server_addr[i])); 

    /* Wait to receive something from the server */
    char buffer[4][48];
    memset(buffer, '\0', sizeof(buffer));

    recv(client_socket[i], buffer[i], 48, 0);
    printf("buffer: %s\n", buffer[i]);
  }
  
  return 0;
}



