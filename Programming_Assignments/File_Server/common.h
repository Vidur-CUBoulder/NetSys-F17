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

#define MAX_BUFFER_LENGTH 6000

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

typedef struct udp_data_packet_t {
  uint32_t file_size;
  uint32_t file_stream_size;
  uint32_t seq_number;
  uint8_t ack_nack;
  uint8_t buffer[MAX_BUFFER_LENGTH+1];
} udp_data_packet;

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

int server_response_1(int sock_fd, struct sockaddr_in remote_socket, \
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



