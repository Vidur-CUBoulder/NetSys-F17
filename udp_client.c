#include "common.h"

#define SERVER_RESPONSE_ENABLED

char global_client_buffer[2][20];

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

void send_client_data(int sock_fd, struct sockaddr_in *remote,\
                          int *packet_count)
{
  /* Send the data to the server */
  int nbytes = 0;
  sendto(sock_fd, packet_count, sizeof(int),\
      0, (struct sockaddr *)remote, (socklen_t)sizeof(struct sockaddr_in));
  if(nbytes < 0) {
    perror("ERROR: sendto()");
    exit(1);
  }

  for (int i = 0; i<(*packet_count); i++) {
    sendto(sock_fd, &global_client_buffer[i], sizeof(global_client_buffer),\
        0, (struct sockaddr *)remote, (socklen_t)sizeof(struct sockaddr_in));
    if(nbytes < 0) {
      perror("ERROR: sendto()");
      exit(1);
    }
  }

  return;
}


void receive_data_from_server(int sock_fd, struct sockaddr_in *remote,\
          udp_data_packet *data_packet, void *filename)
{
  /* Get the total number of packets that the server
   * is going to send back to the client 
   */
  if(filename == NULL) {
    printf("Filename missing!\n");
    return;
  }
  
  /* a. First get the size of the file that is being sent */
  server_response(sock_fd, remote, &data_packet->file_size,\
                      sizeof(data_packet->file_size)); 

  /* b. get the number of packets that are going to be sent */
  
  data_packet->file_stream_size = PACKET_COUNT(data_packet->file_size);
  
  server_response(sock_fd, remote, &(data_packet->file_stream_size),\
      sizeof(data_packet->file_stream_size));
  
  uint32_t packet_count = 0;
  int packet_size = MAX_BUFFER_LENGTH; 
 
  FILE *fp = NULL;
  if(filename == stdout) {
    fp = stdout;
  } else {
    fp = fopen(filename, "wb");
  }
  /* Query for the number of packets that the server is about
   * to send back to the client. The client in turn will reply
   * with an ack/nack every time it receives a stream sent by
   * the server.
   */
  while(packet_count != data_packet->file_stream_size) {
    /* 1. Get the seq. number of the stream */
    server_response(sock_fd, remote, &(data_packet->seq_number),\
                      sizeof(data_packet->seq_number));

    /* 2. Send ACK/NACK */
    data_packet->ack_nack = 1;
    int nbytes = sendto(sock_fd, &(data_packet->ack_nack), sizeof(uint8_t),\
        0, (struct sockaddr *)remote, (socklen_t)sizeof(struct sockaddr_in));
    if(nbytes < 0) {
      perror("ERROR: sendto()");
      exit(1);
    }
    
    /* 3. Wait for the server to send back the stream of data */
    memset(data_packet->buffer, '\0', sizeof(MAX_BUFFER_LENGTH));
    server_response(sock_fd, remote, &(data_packet->buffer),\
        sizeof(data_packet->buffer));

    if(data_packet->seq_number == (data_packet->file_stream_size-1))
    {
      /* This is the last packet; change the size that is written */
      packet_size = (data_packet->file_size)%MAX_BUFFER_LENGTH;
    }
  
    fwrite(&(data_packet->buffer), 1, packet_size, fp);
    packet_count++;
  }
  fclose(fp);
  return;
}

infra_return start_command_infra(int *cntr)
{
  char user_data_buffer[20];
  *cntr = 0;

  printf("Starting the command infra:\n");
  
  /* First get the user data from the command line */
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
  udp_data_packet data_packet;

  /* Start the command infra to get a command from the user */
  while(ret_val != VALID_RETURN) {
    ret_val = start_command_infra(&cntr);
  }

  /* Send the command and the filename entered by the user to the server */
  send_client_data(sock_fd, &remote, &cntr);

#ifdef SERVER_RESPONSE_ENABLED
  if(!strcmp(global_client_buffer[0], valid_commands[3])) {
    receive_data_from_server(sock_fd, &remote, &data_packet, stdout);
  } else {
    char filename[] = "temp_file_1";
    receive_data_from_server(sock_fd, &remote, &data_packet, filename);
  }
#endif
  
  close(sock_fd);

  return 0;
}
