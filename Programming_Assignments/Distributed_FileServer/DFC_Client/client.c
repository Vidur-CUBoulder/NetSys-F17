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
  
#ifdef TEST_SERVER_CONNECTIONS
  Dump_Client_Data(&client_data, 4);
#endif

  /* Make a connection to the 4 servers */
  uint8_t client_socket[MAX_DFS_SERVERS];
  memset(client_socket, '\0', sizeof(client_socket));

  struct sockaddr_in server_addr[MAX_DFS_SERVERS];
  memset(&server_addr, 0, sizeof(server_addr));
    
  for(int i = 0; i<MAX_DFS_SERVERS; i++) {
    Create_Client_Connections(&client_socket[i], client_data.client_ports.port_num[i],\
        &server_addr[i], sizeof(server_addr[i])); 

#ifdef TEST_SERVER_CONNECTIONS
    /* Client receive test */ 
    memset(buffer, '\0', sizeof(buffer));
    recv(client_socket[i], buffer, 50, 0);
    printf("buffer: %s\n", buffer);

    /* Client send test */
    int send_ret = send(client_socket[i], client_data.username,\
        strlen(client_data.username), 0);
    if(send_ret < 0) {
      perror("SEND");
      exit(0);
    }
#endif
    
    infra_return ret = Send_Auth_Client_Login(client_socket[i], &client_data);
    /* If SUCCESS, update the running server list in the client */
    if(ret == AUTH_SUCCESS)
      auth_server_list[i]++;
  }


  int cntr = 0;
  while(1) {
    
    /* Start a command line interface */
    start_command_infra(&cntr, &client_data);
    
    if(!strcmp(global_client_buffer[0], valid_commands[3])) {
      /* Exit from the program, gracefully please! */
      for(int i = 0; i<MAX_DFS_SERVERS; i++)
        send(client_socket[i], "exit", strlen("exit"), 0);
      break;
    } else if(!strcmp(global_client_buffer[0], valid_commands[0])) {
      /* Put the file into the DFS under the 'username' dir. */
      Execute_Put_File(global_client_buffer[1], &client_data);
    } else if(!strcmp(global_client_buffer[0], valid_commands[1])) {
      /* Get the file from the DFS servers */
      printf("In GET!\n");
      Get_File_From_Servers(&client_data);
    } else if(!strcmp(global_client_buffer[0], valid_commands[2])) {
      /* List the files in the DFS and check if its recoverable */
      Execute_List(&client_data);
    } else if(!strcmp(global_client_buffer[0], valid_commands[4])) {
      /* Clear the CLI */
      system("clear");
    } else {
      printf("Please enter a valid command\n");
      break;
    }
  }
 
  /* Close all client connections */
  for(int i = 0; i<MAX_DFS_SERVERS; i++)
    close(client_socket[i]);

  return 0;
}



