#include "common.h"

//#define CLIENT_RESPONSE_ENABLED

char global_client_buffer[2][20];

infra_return validate_input_command(char *input_command)
{
  if(input_command == NULL) {
    return NULL_VALUE;
  }
  
  for(int i = 0; i<(NUM_COMMANDS + 1); i++) {
    if(!strcmp(valid_commands[i], input_command))
      return COMMAND_SUCCESS;
    else
      continue;
  }

  return COMMAND_FAILURE;
}
  

int client_response(int sock_fd, struct sockaddr_in remote_socket, \
                  char *response, int response_size)
{
  /* Block till response from server received ! */ 
  socklen_t addr_len = sizeof(struct sockaddr);
  int nbytes = recvfrom(sock_fd, response, response_size,0,\
      (struct sockaddr *)&remote_socket, &addr_len);    
  if(nbytes < 0) {
    perror("client:RECVFROM()");
    return nbytes;
  }
  
  return nbytes;
}

infra_return start_command_infra(int *cntr)
{
  char user_data_buffer[20];
  //char cli_args[2][20];
  *cntr = 0;

  printf("Starting the command infra:\n");
  
  /* First get the user data from the command line */
  if (fgets(user_data_buffer, sizeof(user_data_buffer), stdin)) {
    char *buffer = strtok(user_data_buffer, " \n"); 
    while(buffer != NULL) {
      //strcpy(cli_args[ctr][20], buffer);
      strcpy(global_client_buffer[*(cntr)], buffer);
      buffer = strtok(NULL, " \n");
      (*cntr)++;
    }
  }

  /* Sanitize input */
  char newline = '\n';
  //if(!strcmp(cli_args[0][20], &newline)) {
  if(!strcmp(global_client_buffer[0], &newline)) {
    printf("inputs not correct. Please try again!\n");
    return INCORRECT_INPUT;
  }

#if 0 
  for(int i = 0; i<2; i++) {
    printf("%s\n", global_client_buffer[i]);
  }
#endif
  
  /* Interpret the command and return the corresponding value
   * the required function.
   */

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
  
  infra_return ret_val;
  int cntr = 0;
  //char cli_args[2][20];
  while(ret_val != VALID_RETURN) {
    ret_val = start_command_infra(&cntr);
  }

#if 0
  for(int i = 0; i<2; i++) {
    printf("%s\n", cli_args[i]);
  } 
#endif
  
  /* Send the data to the server */
  printf("cntr: %d\n", cntr);
  int nbytes = 0;
  sendto(sock_fd, &cntr, sizeof(cntr),\
      0, (struct sockaddr *)&remote, (socklen_t)sizeof(remote));
  if(nbytes < 0) {
    perror("ERROR: sendto()");
    exit(1);
  }

  for (int i = 0; i<cntr; i++) {
    sendto(sock_fd, &global_client_buffer[i], sizeof(global_client_buffer),\
        0, (struct sockaddr *)&remote, (socklen_t)sizeof(remote));
    if(nbytes < 0) {
      perror("ERROR: sendto()");
      exit(1);
    }
  }
  printf("Data successfully sent from the client to the server!\n");

#ifdef CLIENT_RESPONSE_ENABLED
  /* Block till response from server received ! */ 
  char msg[MAX_BUFFER_LENGTH];
  client_response(sock_fd, remote, msg, sizeof(msg));
  printf("return data from the server: %s\n", msg);
#endif
  
  close(sock_fd);

  return 0;
}
