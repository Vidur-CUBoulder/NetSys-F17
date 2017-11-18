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
  int8_t client_socket[MAX_DFS_SERVERS];
  memset(client_socket, '\0', sizeof(client_socket));

  struct sockaddr_in server_addr[MAX_DFS_SERVERS];
  memset(&server_addr, 0, sizeof(server_addr));

  int cntr = 0;
  while(1) {
    
    /* Start a command line interface */
    start_command_infra(&cntr, &client_data);
    
    if(!strcmp(global_client_buffer[0], valid_commands[3])) {
      /* Exit from the program, gracefully please! */
      Authenticate_Client_Connections(client_socket, &client_data, server_addr); 
      for(int i = 0; i<MAX_DFS_SERVERS; i++)
        send(client_socket[i], "exit", strlen("exit"), MSG_NOSIGNAL);
      break;
    } else if(!strcmp(global_client_buffer[0], valid_commands[0])) {
      /* Put the file into the DFS under the 'username' dir. */
      /* Authenticate with all the servers first */
      Authenticate_Client_Connections(client_socket, &client_data, server_addr); 
      
      Execute_Put_File(client_socket, global_client_buffer[1], &client_data);
    
    } else if(!strcmp(global_client_buffer[0], valid_commands[1])) {
      /* Get the file from the DFS servers */
      printf("In GET!\n");
      //Authenticate_Client_Connections(valid_commands[1], client_socket, &client_data); 
      //Get_File_From_Servers(&client_data);
    } else if(!strcmp(global_client_buffer[0], valid_commands[2])) {
      /* List the files in the DFS and check if its recoverable */
      Authenticate_Client_Connections(client_socket, &client_data, server_addr); 
      Execute_List_Client(client_socket, &client_data);
      //Execute_List(&client_data);
    } else if(!strcmp(global_client_buffer[0], valid_commands[4])) {
      /* Clear the CLI */
      system("clear");
    } else {
      printf("Please enter a valid command\n");
      break;
    }

    /* Close all client connections */
    for(int i = 0; i<MAX_DFS_SERVERS; i++)
      close(client_socket[i]);
  }
 
  /* Close all client connections */
  for(int i = 0; i<MAX_DFS_SERVERS; i++)
    close(client_socket[i]);

  return 0;
}



