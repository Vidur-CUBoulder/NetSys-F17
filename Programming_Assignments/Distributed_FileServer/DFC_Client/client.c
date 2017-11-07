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
  uint8_t client_socket[4];
  memset(client_socket, '\0', sizeof(client_socket));

  struct sockaddr_in server_addr[4];
  memset(&server_addr, 0, sizeof(server_addr));
    
  char buffer[MAX_DFS_SERVERS][50];
  memset(buffer, '\0', sizeof(buffer));

  for(int i = 0; i<MAX_DFS_SERVERS; i++) {
    Create_Client_Connections(&client_socket[i], client_data.client_ports.port_num[i],\
        &server_addr[i], sizeof(server_addr[i])); 

#ifdef TEST_SERVER_CONNECTIONS
    /* Client receive test */ 
    char buffer[MAX_DFS_SERVERS][50];
    memset(buffer, '\0', sizeof(buffer));
    recv(client_socket[i], buffer[i], 48, 0);
    printf("buffer: %s\n", buffer[i]);

    /* Client send test */
    int send_ret = send(client_socket[i], client_data.username,\
        strlen(client_data.username), 0);
    if(send_ret < 0) {
      perror("SEND");
      exit(0);
    }
#endif
  
    /* Combine the client data and password in one string and then send that */
    strncat(buffer[i], client_data.username, strlen(client_data.username));
    strncat(buffer[i], " ", strlen(" "));
    strncat(buffer[i], client_data.password, strlen(client_data.password)); 
    //printf("%s\n", buffer[i]);
    
    int send_ret = send(client_socket[i], buffer[i],\
        strlen(buffer[i]), 0);
    if(send_ret < 0) {
      perror("SEND");
      exit(0);
    }
  }
  return 0;


  int cntr = 0;
  while(1) {
    
    /* Start a command line interface */
    start_command_infra(&cntr);
    
    if(!strcmp(global_client_buffer[0], valid_commands[3])) {
      /* Exit from the program, gracefully please! */
      break;
    } else if(!strcmp(global_client_buffer[0], valid_commands[0])) {
      /* Put the file into the DFS under the 'username' dir. */
      Execute_Put_File(global_client_buffer[1]);
    } else if(!strcmp(global_client_buffer[0], valid_commands[1])) {
      /* Get the file from the DFS servers */
    } else if(!strcmp(global_client_buffer[0], valid_commands[2])) {
      /* List the files in the DFS and check if its recoverable */
    } else if(!strcmp(global_client_buffer[0], valid_commands[4])) {
      /* Clear the CLI */
      system("clear");
    } else {
      printf("Please enter a valid command\n");
      break;
    }
  }
  
  return 0;
}



