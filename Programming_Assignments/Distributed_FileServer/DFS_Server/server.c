#include "../common/common.h"

int main(int argc, char *argv[])
{
  switch(argc)
  {
    case 1: /* Config. file not passed */
      printf("Configuration file not passed!\n");
      exit(0);
      break;

    case 2: /* This is the Server name */
      printf("Server name not passed!\n");
      exit(0);
      break;

    case 3: /* This is the port number */
      printf("Port number not passed!\n");
      exit(0);
      break;

    default:/* Do nothing in this case */
      break;
  }

  /* Sanitize the DFS server names passed! */
  if(strcmp(argv[2], "DFS1") && strcmp(argv[2], "DFS2") &&\
      strcmp(argv[2],"DFS3") && strcmp(argv[2], "DFS4")) {
    printf("Invalid DFS Server name!\n");
  }

  /* Parse the config file and store its contents */
  server_config_data_t server_config;
  Parse_Server_Config_File(argv[1], &server_config);

  /* Get the port number and setup the server */
  uint16_t port_num = atoi((argv[3]));
  int server_socket = 0;

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));

  socklen_t addr_len = sizeof(address);
  int accept_ret = 0;

  char buffer[50];
  memset(buffer, '\0', sizeof(buffer));

  static int server_counter = 0;

  infra_return ret_val = 0;

#if 1
  while(1) {

    Create_Server_Connections(&server_socket, &address,\
      sizeof(address), port_num);
    
    //accept(server_socket, (struct sockaddr *)&address, &addr_len);
    infra_return r_val = Accept_Auth_Client_Connections(&accept_ret, server_socket,\
        &address, addr_len, &server_config );
    
    /* Get the data from the client and verify username and pass */
    memset(buffer, '\0', sizeof(buffer));
    int recv_ret = recv(accept_ret, buffer, 50, 0);
    if(recv_ret < 0) {
      perror("");
      return 0;
    }
  
    /* The recv for the 2nd CLI from user, extra dir. one */
    char dir_name[10];
    memset(dir_name, '\0', sizeof(dir_name));
    
    size_t dir_name_size = 0;
    recv(accept_ret, &dir_name_size, sizeof(size_t), 0);
    
    if(dir_name_size != 0) {
      recv(accept_ret, &dir_name, dir_name_size, 0);
      memset(server_config.username[0], '\0', sizeof(server_config.username[0]));
      memcpy(server_config.username[0], dir_name, strlen(dir_name));
    }


    if(!(strcmp(buffer, "exit"))) {
      break;
    } else if (!strcmp(buffer, valid_commands[0])) { 
      
      Execute_Put_Server(&accept_ret, &server_config);
    } else if(!strcmp(buffer, valid_commands[1])) {
      char temp[4];
      memcpy(temp, argv[2], strlen(argv[2]));
      removeSubstring(temp, "DFS");
      int out = atoi(temp);
      
      uint8_t run_next = 0;    
      recv(accept_ret, &run_next, sizeof(uint8_t), 0);
      if(run_next >= 1) 
        Give_File_To_Client(&accept_ret, &server_config, argv[2], (out-1));
    } else if(!strcmp(buffer, valid_commands[2])) {
      Execute_List_Server(&accept_ret, argv[2], &server_config);
    } else {
      printf("Invalid username sent!\n");
      break;
    }
#endif
    /* Gracefully shutdown the socket connection */ 
    shutdown(accept_ret, SHUT_RDWR);  
    close(server_socket);
  
  }

  shutdown(accept_ret, SHUT_RDWR);  
  close(server_socket);

  return 0;
}

