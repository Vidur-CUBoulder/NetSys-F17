#include "common.h"

int server_response(int sock_fd, struct sockaddr_in *remote_socket, \
                  void *response, int response_size)
{
  /* Block till response from server received ! */ 
  socklen_t addr_len = sizeof(struct sockaddr);
  int nbytes = recvfrom(sock_fd, response, response_size,0,\
      (struct sockaddr *)remote_socket, &addr_len);    
  if(nbytes < 0) {
    perror("client:RECVFROM()");
    return nbytes;
  }
   
  return nbytes;
}

void client_send_CLI_data(int sock_fd, struct sockaddr_in *remote,\
                          int *packet_count)
{
  /* Send the data to the server */
  int nbytes = 0;
  sendto_wrapper(sock_fd, *remote, packet_count, sizeof(int));

  
  for (int i = 0; i<(*packet_count); i++) {
    sendto_wrapper(sock_fd, *remote, &global_client_buffer[i],\
        sizeof(global_client_buffer[i]));
  }

  return;
}

infra_return start_command_infra(int *cntr)
{
  char user_data_buffer[20] = {0};
  *cntr = 0;
  
  /* First get the user data from the command line */
  printf("> ");
  if (fgets(user_data_buffer, sizeof(user_data_buffer), stdin)) {
    char *buffer = strtok(user_data_buffer, " \n"); 
    while(buffer != NULL) {
      strcpy(global_client_buffer[*(cntr)], buffer);
      buffer = strtok(NULL, " \n");
      (*cntr)++;
    }
  }

  /* Sanitize input */
  char newline = '\n';
  if(!strcmp(global_client_buffer[0], &newline)) {
    printf("inputs not correct. Please try again!\n");
    return INCORRECT_INPUT;
  }

  if(!strcmp(global_client_buffer[0],valid_commands[4])) {
    return COMMAND_EXIT;
  }

  return VALID_RETURN;
}



int main(int argc, char *argv[])
{
  int sock_fd = 0;  
  struct sockaddr_in remote;

  /* check the arguments passed from the command line */
  if(argc < 3) {
    printf("USAGE: <server ip> <server port>\n");
    exit(1);
  }
  
  sock_fd = populate_socket_addr(&remote, AF_INET, htons(atoi(argv[2])),\
                        (inet_addr(argv[1])));
  
  setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&sock_timeout, sizeof(sock_timeout));
  
  infra_return ret_val = 0;
  int cntr = 0;

  /* Start the command infra to get a command from the user */
  while(1) 
  {
    ret_val = 0;
    ret_val = start_command_infra(&cntr);
    
    /* Send the command and the filename entered by the user to the server */
    client_send_CLI_data(sock_fd, &remote, &cntr);
    
    if(ret_val == COMMAND_EXIT) {
      printf("Terminating the client program!\n");
      break;
    }
    
#ifdef RESPONSE_ENABLED
    if(!strcmp(global_client_buffer[0], valid_commands[3])) {
      receive_file(sock_fd, &remote, stdout);
    } else if(!strcmp(global_client_buffer[0], valid_commands[0])) {
      send_file(global_client_buffer[1], sock_fd, &remote);
      printf("%s sent!\n", global_client_buffer[1]);
    } else if(!strcmp(global_client_buffer[0], valid_commands[4])) {
      printf("Terminating the Client program!\n");
      break;
    } else if(!strcmp(global_client_buffer[0], valid_commands[1])) {
      receive_file(sock_fd, &remote, global_client_buffer[1]);
      printf("%s received!\n", global_client_buffer[1]);
    } else if(!strcmp(global_client_buffer[0], valid_commands[5])) {
      system("clear");
    } else {
      continue;
    }
    
    memset(global_client_buffer, '\0', sizeof(global_client_buffer));
#endif
  }

  close(sock_fd);

  return 0;
}
