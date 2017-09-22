#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include<openssl/crypto.h>
#include<openssl/md5.h>

#define SOCKET_TIMEOUT

#define MAX_BUFFER_LENGTH 5000

#define UDP_SOCKETS SOCK_DGRAM

#define NUM_COMMANDS 5

#define PACKET_COUNT(x) ((x%MAX_BUFFER_LENGTH) == 0) ?\
                      (x/MAX_BUFFER_LENGTH) : ((x/MAX_BUFFER_LENGTH) + 1)

#define RESPONSE_ENABLED

char global_client_buffer[2][20];
char global_server_buffer[2][20];


typedef enum infra_return_e {
  NULL_VALUE = 0,
  INCORRECT_INPUT,
  VALID_RETURN,
  COMMAND_SUCCESS, 
  COMMAND_FAILURE,
  COMMAND_EXIT,
  COMMAND_CLEAR_SCREEN
} infra_return;

typedef enum valid_client_commands_e {
  PUT = 0,
  GET, 
  DELETE,
  LS,
  EXIT,
  CLEAR
} valid_client_commands;


char *valid_commands[] = {  
  "put", 
  "get",
  "delete",
  "ls",
  "exit",
  "clear"
};

typedef struct __data_buffer_t {
  uint32_t seq_number;
  uint8_t buffer[MAX_BUFFER_LENGTH+1];
} data_buffer_t;

typedef struct udp_data_packet_t {
  uint32_t file_size;
  uint32_t file_stream_size;
  //uint32_t seq_number;
  uint8_t ack_nack;
  data_buffer_t data_buffer;
} udp_data_packet;

struct timeval sock_timeout;

#if ENABLE_MD5_HASHING
void create_md5_hash(char *digest_buffer, char *buffer, int buffer_size)
{
  char digest[16];
  MD5_CTX md5_struct;
  
  MD5_Init(&md5_struct);
  MD5_Update(&md5_struct, buffer, buffer_size);
  MD5_Final(digest, &md5_struct);
  
  return;
}
#endif

int populate_socket_addr(struct sockaddr_in *sock, uint8_t sock_family,\
                            uint16_t sock_port, in_addr_t sock_addr)
{
  bzero(sock, sizeof(struct sockaddr_in)); 
  sock->sin_family = sock_family;
  sock->sin_port = sock_port;
  sock->sin_addr.s_addr = sock_addr;

  int sock_fd = socket(sock_family, UDP_SOCKETS, 0);
  if(sock_fd < 0) {
    perror("ERROR:socket()");
    exit(1);
  }
  return sock_fd;
}

int recvfrom_wrapper(int sock_fd, struct sockaddr_in remote, \
            void *response, int response_size)
{
  int nbytes = recvfrom(sock_fd, response, response_size,\
      0, (struct sockaddr *)&remote, &response_size);
  if(nbytes < 0) {
    perror("ERROR:recvfrom()");
  }

  return nbytes;
}

int client_response(int sock_fd, struct sockaddr_in remote_socket, \
            void *response, int response_size, int remote_length)
{
  int nbytes = recvfrom(sock_fd, response, response_size, 0,\
                (struct sockaddr *)&remote_socket,\
                &remote_length);
  if(nbytes < 0) {
    perror("server:SENDTO");
    return nbytes;
  }

  return nbytes;
}

int sendto_wrapper(int sock_fd, struct sockaddr_in remote_socket, \
            void *response, int response_size)
{
  int nbytes = sendto(sock_fd, response, response_size, 0,\
                (struct sockaddr *)&remote_socket,\
                (socklen_t)sizeof(struct sockaddr_in));
  if(nbytes < 0) {
    perror("server:SENDTO");
  }

  return nbytes;
}

void send_file(char *filename, int sock_fd,\
                struct sockaddr_in *remote)
{
  if(filename == NULL) {  
    printf("Please specify a filename to send to the server\n");
    return;
  }

  /* initialize and clear the data_packet structure */
  udp_data_packet data_packet;
  memset(&data_packet, 0, sizeof(data_packet));

  FILE *fp;
  fp = fopen(filename, "rb");
  int packet_size = MAX_BUFFER_LENGTH;
  int packet_count = 1;
  int nbytes = 0;
  int setsock_return = 0;
  int32_t remote_length = sizeof(remote);
  //uint8_t send_buffer[MAX_BUFFER_LENGTH + 1 + sizeof(data_packet.seq_number)];


  /* Find the size of the file and send that information to the server */
  fseek(fp, 0L, SEEK_END);
  data_packet.file_size = ftell(fp);
  data_packet.file_stream_size = PACKET_COUNT(data_packet.file_size);
  
  /* Send the file_size to the receiver */
  sendto_wrapper(sock_fd, *remote, &data_packet.file_size,\
            sizeof(data_packet.file_size));
  
  /* Reset the file pointer */
  rewind(fp);

  /* Send the receiver the number of packets that it should expect */
  sendto_wrapper(sock_fd, *remote, &(data_packet.file_stream_size),\
      sizeof(data_packet.file_stream_size));

  /* Start reading the file that has to be sent to the receiver.
   * The data read will be stored in the buffer defined as part of 
   * the struct udp_data_packet.
   */
  data_packet.data_buffer.seq_number = 1;
  
  if(fp != NULL) {
    while(fread(&(data_packet.data_buffer.buffer), 1, packet_size, fp) > 0) 
    {
      /* 1. Send the seq. number along with the chunk of data that 
       *    was read into the buffer.
       * 2. Start the timeout.
       * 3. Wait for the ack to be sent by the receiver.
       * 4. If ACK, continue. Else, wait for the timeout to expire and resend
       */

      /* Reset the ack_nack and start the wait for an ACK */
      data_packet.ack_nack = 0; 
      
      while(data_packet.ack_nack != 1) {
        /* Now send the buffer to the receiver */
        sock_timeout.tv_sec = 1; 
        sock_timeout.tv_usec = 2000; 
        
        sendto_wrapper(sock_fd, *remote, &(data_packet.data_buffer),\
            sizeof(data_packet.data_buffer));

        /* Wait for the ACK! 
         * --> if the ACK is received before the timeout, stop the timer 
         * --> else just wait here until the ACK is received!
         */

        /* get the ACK from the receiver! */
        nbytes = recvfrom_wrapper(sock_fd, *remote, &(data_packet.ack_nack),\
            sizeof(data_packet.ack_nack));
        
        if(nbytes < 0) {
          /* Time out, resend the packet! */
          continue;
        } else { 
          if(data_packet.ack_nack == 1) {
            printf("ACK received!; seq_number: %d; packet_count: %d\n", data_packet.data_buffer.seq_number, packet_count);
          } else {
            /* Resend the prev. packet */
            printf("\n\nNACK received!; seq_number: %d; packet_count: %d\n", data_packet.data_buffer.seq_number, packet_count);
          }
        }
      
      }    
      
      /* Clean up */
      memset(&data_packet.data_buffer.buffer, '\0',\
          sizeof(data_packet.data_buffer.buffer));
      
      data_packet.data_buffer.seq_number = ++packet_count;
      
      if(data_packet.data_buffer.seq_number == data_packet.file_stream_size)
      {
        /* This is the last packet; change the size that is read */
        packet_size = data_packet.file_size%MAX_BUFFER_LENGTH;
      }
    }
  } else {
    printf("Unable to open the file!\n");
  }
  
  /* Close the file once you're done */
  fclose(fp);

  return;
}


void receive_file(int sock_fd, struct sockaddr_in *remote, void *filename)
{
  udp_data_packet data_packet; 
  memset(&data_packet, 0, sizeof(data_packet)); 
  
  int nbytes = 0;
  int32_t remote_length = sizeof(remote);
  uint32_t packet_count = 1;
  int packet_size = MAX_BUFFER_LENGTH;

  /* Get the size of the file from the sender */
  nbytes = recvfrom_wrapper(sock_fd, *remote, &(data_packet.file_size),\
                      sizeof(data_packet.file_size));

  /* Get the number of packets that the send will be sending over */
  nbytes = recvfrom_wrapper(sock_fd, *remote, &(data_packet.file_stream_size),\
                        sizeof(data_packet.file_stream_size));

  FILE *fp = (filename == stdout) ? stdout : fopen(filename, "wb");
  
  /* 1.Get the data that the sender sends out
   * 2. once received, send out the ACK.
   * 3. Continue this until the packet_count isn't equal to the
   *    expected number of packets that the sender promised to send out
   */
  while(packet_count != (data_packet.file_stream_size+1))
  {
    
    printf("\n\nstream_size: %d; packet_count: %d\n",\
        data_packet.file_stream_size, packet_count);

    /* Get the packet first and then strip the seq number from it */
    recvfrom_wrapper(sock_fd, *remote, &data_packet.data_buffer, \
                  sizeof(data_packet.data_buffer));

    printf("Stripped seq number is: %d\n", data_packet.data_buffer.seq_number);
    
    /* Send the ACK to the sender! */
    if(data_packet.data_buffer.seq_number == packet_count) {
      data_packet.ack_nack = 1;
      sendto_wrapper(sock_fd, *remote, &(data_packet.ack_nack),\
          sizeof(data_packet.ack_nack));
    } else {
      /* This implies that the ack was lost */
      data_packet.ack_nack = 0;
      sendto_wrapper(sock_fd, *remote, &(data_packet.ack_nack),\
          sizeof(data_packet.ack_nack));
      continue;
    }

    if(data_packet.data_buffer.seq_number == (data_packet.file_stream_size))
    {
      /* This is the last packet; change the size that is written */
      packet_size = (data_packet.file_size)%MAX_BUFFER_LENGTH;
      printf("last packet_size: %d\n", packet_size);
    }
    
    /* Write the data into the file */
    fwrite(&(data_packet.data_buffer.buffer), 1, packet_size, fp);
    packet_count++;
    
    /* Clean up */
    memset(&(data_packet.data_buffer.buffer), 0, sizeof(data_packet.data_buffer.buffer));
  }

  /* Close the file once you're done with it */
  if(filename != stdout)
    fclose(fp);

  return;
}

