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


void send_file_from_client(char *filename,int sock_fd,\
                    struct sockaddr_in *remote)
{
  if(filename == NULL) {
    printf("Please specify a filename to send to the server\n");
    return;
  }

  udp_data_packet client_data_packet;
  memset(&client_data_packet.buffer, '\0', sizeof(client_data_packet.buffer));
  
  printf("global_client_buffer[1]: %s\n", filename);

  FILE *fp;
  fp = fopen(filename, "rb");
  int packet_size = MAX_BUFFER_LENGTH;
  int pkt_count = 0;
  unsigned int remote_length = 0;

  /* Find the size of the file and send that information to the server */
  fseek(fp, 0L, SEEK_END);
  client_data_packet.file_size = ftell(fp);
  client_data_packet.file_stream_size =\
            PACKET_COUNT(client_data_packet.file_size);
  
  printf("file_size: %d\n", client_data_packet.file_size);
  /* Send the size of the file over to the server */

  server_response_1(sock_fd, *remote, &client_data_packet.file_size,\
            sizeof(client_data_packet.file_size));

  rewind(fp);
  
  server_response_1(sock_fd, *remote, &client_data_packet.file_stream_size,\
      sizeof(client_data_packet.file_stream_size));
 
  printf("file_stream_size: %d\n", client_data_packet.file_stream_size);

  if(fp != NULL) {
    while(fread(&(client_data_packet.buffer), 1, packet_size, fp) > 0) 
    {
      /* Send the seq number of the packet first and wait for ack */
      server_response_1(sock_fd, *remote, &(client_data_packet.seq_number),\
          sizeof(client_data_packet.seq_number));

      /* Wait for an ack! */
      remote_length = sizeof(remote);
      int nbytes = recvfrom(sock_fd, &client_data_packet.ack_nack, sizeof(client_data_packet.ack_nack),\
          0, (struct sockaddr *)remote, &remote_length);
      if(nbytes < 0) {
        perror("ERROR:recvfrom()");
      }
     
      server_response_1(sock_fd, *remote, &client_data_packet.buffer,\
          sizeof(client_data_packet.buffer));

      /* Send the data packet and wait for confirmation from the client */
      client_data_packet.seq_number = ++pkt_count;
      
      if(client_data_packet.seq_number == client_data_packet.file_stream_size)
      {
        /* This is the last packet; change the size that is read */
        packet_size = client_data_packet.file_size%MAX_BUFFER_LENGTH;
      }
    }  
  } else {
    printf("Unable to open the file for reading\n");
  }
    
  fclose(fp);

  return;
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
  uint32_t packet_size = MAX_BUFFER_LENGTH; 
 
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
  while(packet_count != (data_packet->file_stream_size)) {
  
    printf("\npacket_count: %d\n", packet_count);

    /* 1. Get the seq. number of the stream */
    server_response(sock_fd, remote, &(data_packet->seq_number),\
                      sizeof(data_packet->seq_number));
    
    printf("seq_number: %d\n", data_packet->seq_number);
    
    /* 2. Send ACK/NACK */
    data_packet->ack_nack = (data_packet->seq_number == packet_count) ? 1 : 0;
    
    int nbytes = sendto(sock_fd, &(data_packet->ack_nack), sizeof(data_packet->ack_nack),\
        0, (struct sockaddr *)remote, (socklen_t)sizeof(struct sockaddr_in));
    if(nbytes < 0) {
      perror("ERROR: sendto()");
      exit(1);
    }
    if(!data_packet->ack_nack) {
      /* Re-send the packet */
      printf("Resending the packet!\n");
      server_response(sock_fd, remote, &(data_packet->buffer),\
        sizeof(data_packet->buffer));
      printf("\n%s\n", data_packet->buffer);
      getchar();
    } 
    
    /* 3. Wait for the server to send back the stream of data */
    memset(data_packet->buffer, '\0', MAX_BUFFER_LENGTH);
    
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
  
  if(filename != stdout)
    fclose(fp);
  
  return;
}

infra_return start_command_infra(int *cntr)
{
  char user_data_buffer[20];
  *cntr = 0;
  
  /* First get the user data from the command line */
  printf("starting the command infra:\n");
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
  while(1) 
  {
    ret_val = 0;
    ret_val = start_command_infra(&cntr);
    
    /* Send the command and the filename entered by the user to the server */
    send_client_data(sock_fd, &remote, &cntr);
    
    if(ret_val == COMMAND_EXIT)
      break;

    
#ifdef RESPONSE_ENABLED
    if(!strcmp(global_client_buffer[0], valid_commands[3])) {
      receive_data_from_server(sock_fd, &remote, &data_packet, stdout);
    } else if(!strcmp(global_client_buffer[0], valid_commands[0])) {
      send_file_from_client(global_client_buffer[1], sock_fd, &remote);
    } else if(!strcmp(global_client_buffer[0], valid_commands[4])) {
      printf("Terminating the Client program!\n");
      break;
    } else if(!strcmp(global_client_buffer[0], valid_commands[1])) {
      printf("Command is: Get!\n");
      char filename[] = "temp_file_1";
      receive_data_from_server(sock_fd, &remote, &data_packet, filename);
    } else if(!strcmp(global_client_buffer[0], valid_commands[5])) {
      system("clear");
    } else {
      continue;
    }
#endif
    
    memset(global_client_buffer, '\0', sizeof(global_client_buffer));
  }
  
  close(sock_fd);

  return 0;
}
