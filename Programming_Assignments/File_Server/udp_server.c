#include "common.h"

int server_response(int sock_fd, struct sockaddr_in remote_socket, \
            void *response, int response_size)
{
  int nbytes = sendto(sock_fd, response, response_size, 0,\
                (struct sockaddr *)&remote_socket,\
                (socklen_t)sizeof(struct sockaddr_in));
  if(nbytes < 0) {
    perror("server:SENDTO");
    return nbytes;
  }

  return nbytes;
}

void execute_ls(int sock_fd, struct sockaddr_in *remote)
{
  /* execute the ls command and store the output in a file */
  system("ls | grep -v \"ls_outfile\" > ls_outfile"); 

  //send_data_to_client("ls_outfile", sock_fd, remote);
  send_file("ls_outfile", sock_fd, remote);

  system("rm ls_outfile");

  return;
}


void server_receive_CLI_data(int sock_fd, struct sockaddr_in *remote, int *packet_count)
{
  /* Create a buffer to store the incoming data from the client in */
  int nbytes = 0;
  unsigned int remote_length = sizeof(remote);
  
  /* call the recvfrom call and block on data from the client */
   
  nbytes = recvfrom(sock_fd, packet_count, sizeof(int),\
      0, (struct sockaddr *)remote, &remote_length);
  if(nbytes < 0) {
    perror("ERROR:recvfrom()");
  }

  printf("data from client: %d\n", *packet_count);

  for(int i = 0; i<(*packet_count); i++) { 
    nbytes = recvfrom(sock_fd, &global_server_buffer[i], sizeof(global_server_buffer),\
        0, (struct sockaddr *)remote, &remote_length);
    if(nbytes < 0) {
      perror("ERROR:recvfrom()");
    }
    printf("data from client: %s\n", global_server_buffer[i]);
  }

  return;
}

infra_return validate_input_command(char *input_command, int *num)
{
  if(input_command == NULL) {
    return NULL_VALUE;
  }
  
  for(int i = 0; i<(NUM_COMMANDS); i++) {
    if(!strcmp(valid_commands[i], input_command)) {
      *num = i;
      return COMMAND_SUCCESS;
    } else {
      continue;
    }
  }
  
  return COMMAND_FAILURE;
}

infra_return execute_client_commands(int sock_fd, struct sockaddr_in *remote)
{
#if 1
  printf("command: %s\n", global_server_buffer[0]);
  printf("filename: %s\n", global_server_buffer[1]);
#endif
  
  int command_num = 0;
  int ret_val = 0;
  if(validate_input_command(global_server_buffer[0], &command_num) == COMMAND_SUCCESS) {
    switch((valid_client_commands)command_num)
    {
      case PUT:     
                    //receive the transmitted file from the client.
                    printf("In PUT!\n");
                    //get_data_from_client(sock_fd, remote);
                    receive_file(sock_fd, remote, "silly_file");
                    break;

      case GET:     
                    //send the requested file to the client
                    /* TODO: Check if the file is present in the currect directory */
                    printf("In GET!\n"); 
                    printf("global_server_buffer[1]: %s\n", global_server_buffer[1]);
                    send_file(global_server_buffer[1], sock_fd, remote);
                    //send_data_to_client(global_server_buffer[1], sock_fd, remote); 
                    break;

      case DELETE:  
                    //delete the file if found
                    ret_val = remove(global_server_buffer[1]);
                    if(ret_val) {
                      /* TODO: need to send this data over to the client */
                      printf("Could not remove the file\n");
                    }
                    break;

      case LS:      
                    //send a list of the files in the current dir.
                    execute_ls(sock_fd, remote);
                    break;

      case EXIT:    
                    //gracefully exit from the program
                    printf("Terminating the Server program!\n");
                    return COMMAND_EXIT;
                    break;

      default:      //this is just default.
                    return COMMAND_EXIT;
    }
    return COMMAND_SUCCESS;
  } else {
    printf("invalid command passed!\n");
    return COMMAND_FAILURE;
  }
  
  return COMMAND_FAILURE;
}


int main (int argc, char * argv[] )
{
  int sock_fd = 0;
  struct sockaddr_in server_socket, remote;

  /* check if arguments were passed from the command line interface */
  if(argc != 2) {
    printf("Usage: <port number>\n");
    exit(1);
  }
 
  sock_fd = populate_socket_addr(&server_socket, AF_INET, htons(atoi(argv[1])),\
                          INADDR_ANY);
  
  /* Bind the created socket to the provided address and port number */
  int bind_ret = bind(sock_fd, (struct sockaddr *)&server_socket,\
                      sizeof(server_socket));
  if(bind_ret < 0) {
    perror("ERROR:bind()");
    exit(1);
  }

  /* Wait for the client to send data */
  int packet_count = 0; 
  infra_return retval = 0;

  while(1) { 

    /* config. the timeout values. */
    retval = 0;
    packet_count = 0; 
    memset(global_server_buffer, '\0', sizeof(global_server_buffer));

    server_receive_CLI_data(sock_fd, &remote, &packet_count);
    retval = execute_client_commands(sock_fd, &remote);
    
    if(retval == COMMAND_EXIT) 
      break;

  }

  close(sock_fd);

  return 0;
}

