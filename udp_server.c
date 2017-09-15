#include "common.h"

#define SERVER_RESPONSE_ENABLED

char global_server_buffer[2][20];

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

void send_data_to_client(char *filename, int sock_fd,\
                          struct sockaddr_in *remote)
{
  if(filename == NULL) {
    printf("file not specified!\n");
    return;
  }
  
  udp_data_packet udp_packet;
  memset(&udp_packet.buffer, '\0', sizeof(udp_packet.buffer));
  FILE *fp;
  fp = fopen(filename, "rb");
  int pkt_count = 0;
  unsigned int remote_length = 0;
  
  /* Find the size of the file and send that information to the client */
  fseek(fp, 0L, SEEK_END);
  udp_packet.file_size = ftell(fp);
  udp_packet.file_stream_size = PACKET_COUNT(udp_packet.file_size);

  /* Need to account for the data in the last packet */
  int packet_size = MAX_BUFFER_LENGTH;
  
  /* Send the size of the file over to the client */
  server_response(sock_fd, *remote, &udp_packet.file_size, sizeof(udp_packet.file_size));
  
  /* Reset the file pointer */
  rewind(fp);
  
  /* Number of packets that will be transmitted to the client */
  server_response(sock_fd, *remote, &(udp_packet.file_stream_size),\
      sizeof(udp_packet.file_stream_size));

  if(fp != NULL) {
    while(fread(&(udp_packet.buffer), 1, packet_size, fp) > 0) 
    {
      /* Send the seq number of the packet first and wait for ack */
      server_response(sock_fd, *remote, &(udp_packet.seq_number),\
                        sizeof(udp_packet.seq_number));
     
      /* Wait for an ack! */
      remote_length = sizeof(remote);
      int nbytes = recvfrom(sock_fd, &udp_packet.ack_nack, sizeof(uint8_t),\
          0, (struct sockaddr *)remote, &remote_length);
      if(nbytes < 0) {
        perror("ERROR:recvfrom()");
      }

      /* Send the data buffer over to the client and clear the buffer 
       * in preparation for the next stream of data to be sent.
       */
      server_response(sock_fd, *remote, &(udp_packet.buffer),\
                      MAX_BUFFER_LENGTH);
      
      memset(&udp_packet.buffer, '\0', sizeof(udp_packet.buffer));
      
      /* Send the data packet and wait for confirmation from the client */
      udp_packet.seq_number = ++pkt_count;
      
      if(udp_packet.seq_number == udp_packet.file_stream_size)
      {
        /* This is the last packet; change the size that is read */
        packet_size = udp_packet.file_size%MAX_BUFFER_LENGTH;
      }
    }
  } else {
    printf("Unable to open the file\n");
  }

  return;
}
       

void execute_ls(int sock_fd, struct sockaddr_in *remote)
{
  /* execute the ls command and store the output in a file */
  system("ls > ls_outfile"); 

  send_data_to_client("ls_outfile", sock_fd, remote);

  system("rm ls_outfile");

  return;
}


void receive_client_data(int sock_fd, struct sockaddr_in *remote, int *packet_count)
{
  /* Create a buffer to store the incoming data from the client in */
 unsigned int remote_length = sizeof(remote);
  
  /* call the recvfrom call and block on data from the client */
  int nbytes = recvfrom(sock_fd, packet_count, sizeof(int),\
      0, (struct sockaddr *)remote, &remote_length);
  if(nbytes < 0) {
    perror("ERROR:recvfrom()");
  }
  printf("packet_count: %d\n", *packet_count);

  for(int i = 0; i<(*packet_count); i++) { 
    nbytes = recvfrom(sock_fd, &global_server_buffer[i], sizeof(global_server_buffer),\
        0, (struct sockaddr *)remote, &remote_length);
    if(nbytes < 0) {
      perror("ERROR:recvfrom()");
    }
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

void execute_client_commands(int sock_fd, struct sockaddr_in *remote)
{
  printf("command: %s\n", global_server_buffer[0]);
  printf("filename: %s\n", global_server_buffer[1]);

  int command_num = 0;
  if(validate_input_command(global_server_buffer[0], &command_num)==\
                                COMMAND_SUCCESS) {
    switch((valid_client_commands)command_num)
    {
      case PUT:     printf("put\n");
                    //receive the transmitted file from the client.
                    printf("in the PUT statement!\n");
                    //get_filedata_from_server("temp_file", sock_fd, remote);  
                    break;

      case GET:     printf("get\n");
                    //send the requested file to the client
                    /* TODO: Check if the file is present in the currect directory */
                    send_data_to_client(global_server_buffer[1], sock_fd, remote); 
                    break;

      case DELETE:  printf("delete\n");
                    //delete the file if found
                    break;

      case LS:      printf("ls\n");
                    //send a list of the files in the current dir.
                    execute_ls(sock_fd, remote);
                    break;

      case EXIT:    printf("exit\n");
                    //gracefully exit from the program
                    break;

      default:      //this is just default.
                    printf("nothing!\n");
    }
  } else {
    printf("invalid command passed!\n");
  }
  
  return;
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
  receive_client_data(sock_fd, &remote, &packet_count);

  execute_client_commands(sock_fd, &remote);

#ifdef SERVER_RESPONSE_ENABLED
  /* Send a response back to the client */
  char msg[] = "oranges"; 
  server_response(sock_fd, remote, msg, sizeof(msg));

#endif
  
  close(sock_fd);

  return 0;
}

