#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<stdbool.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<unistd.h>
#include<inttypes.h>
#include<sys/wait.h>
#include<openssl/crypto.h>
#include<openssl/md5.h>
#include<errno.h>
#include<limits.h>
#include<sys/stat.h>
#include<dirent.h>

#define TCP_SOCKETS SOCK_STREAM

#define MAX_DFS_SERVERS 4

#define CHUNK_DUPLICATES 2

#define ENCRYPTION_ENABLED

typedef enum infra_return_e {
  NULL_VALUE = 0,
  INCORRECT_INPUT,
  VALID_RETURN,
  COMMAND_SUCCESS, 
  COMMAND_FAILURE,
  COMMAND_EXIT,
  AUTH_FAILURE,
  AUTH_SUCCESS,
  COMMAND_CLEAR_SCREEN
} infra_return;

typedef struct __client_IP_and_Port_t {
  char IP_Addr[10][10];
  uint16_t port_num[10];
} client_ip_port_t;

typedef struct __client_config_data_t {
  char DFS_Server_Names[10][10];
  client_ip_port_t client_ports;
  char filename[20];
  char username[15];
  char password[15];
} client_config_data_t;

typedef struct __server_config_data_t {
  char username[5][15];
  char password[5][15];
  char filename[5][20];
} server_config_data_t;

char chunk_filename_list[4][20];

char global_client_buffer[2][20];

uint8_t put_file_count = 0;
char cache_put_filenames[5][20];

/* Index 0 --> DFS1
 * Index 1 --> DFS2 and so on..
 */
uint8_t auth_server_list[MAX_DFS_SERVERS];

char chunk_names[MAX_DFS_SERVERS][10] = {
  "chunk_0", "chunk_1", "chunk_2", "chunk_3"
  };

uint8_t Distribution_Schema[MAX_DFS_SERVERS][MAX_DFS_SERVERS][CHUNK_DUPLICATES] = {
/*0*/  {{1, 4}, {1, 2}, {2, 3}, {3, 4}},\
/*1*/  {{1, 2}, {2, 3}, {3, 4}, {4, 1}},\
/*2*/  {{2, 3}, {3, 4}, {4, 1}, {1, 2}},\
/*3*/  {{3, 4}, {4, 1}, {1, 2}, {2, 3}}
};

char *valid_commands[] = {  
  "put", 
  "get",
  "list",
  "exit",
  "clear"
};

infra_return Validate_Login_Credentials(char *input_buffer,\
                  server_config_data_t *config_data, uint8_t *server_count)
{
  if(input_buffer == NULL) {
    printf("<<%s>>: Null Value passed!\n", __func__);
    return NULL_VALUE;
  }
   
  char *local_buffer = strtok(input_buffer, " \n");
  while(local_buffer != NULL) {
    if(strcmp(local_buffer, config_data->username[0])) {// auth. failed! 
      return AUTH_FAILURE;
    } else {
      local_buffer = strtok(NULL, " \n");
      printf("<<%s>>: %s\n", __func__, local_buffer);
      if(strcmp(local_buffer, config_data->password[0])) {
        return AUTH_FAILURE;
      } else {
        /* Tell the client that all auth was successful */
        return AUTH_SUCCESS;
      
      }
    }
    local_buffer = strtok(NULL, " \n");
  }
}

infra_return Send_Auth_Client_Login(int client_socket, client_config_data_t *client_data)
{

  char buffer[50];
  memset(buffer, '\0', sizeof(buffer));

  /* Combine the client data and password in one string and then send that */
  strncat(buffer, client_data->username, strlen(client_data->username));
  strncat(buffer, " ", strlen(" "));
  strncat(buffer, client_data->password, strlen(client_data->password)); 

  int send_ret = send(client_socket, buffer, strlen(buffer), MSG_NOSIGNAL);
  if(send_ret < 0) {
    perror("SEND");
    return AUTH_FAILURE;
  }
  
  /* Receive pass/fail status from server */
  memset(buffer, '\0', sizeof(buffer));
  recv(client_socket, buffer, 50, 0);
  //printf("<%s>:buffer: %s\n", __func__, buffer);
 
  if(!strcmp(buffer, "fail")) { /* Auth Failed! */
    /* Don't close connection, just decline and exit */
    return AUTH_FAILURE;
  } else { /* Auth Passed! */
    printf("<%s>: Auth Passed!\n", __func__);
    return AUTH_SUCCESS;
  }
}

infra_return start_command_infra(int *cntr, client_config_data_t *client_data)
{
  char user_data_buffer[20];
  *cntr = 0;
 
  memset(global_client_buffer, '\0', sizeof(global_client_buffer));

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

  if(global_client_buffer[1] != NULL) {

    char local_buffer[40];
    memset(local_buffer, '\0', sizeof(local_buffer));

    strcpy(local_buffer, global_client_buffer[1]);

    char *buffer = strtok(local_buffer, "/\n");
    while(buffer != NULL) {
      /* the last thing on the command line should be the filename target! */ 
      memset(client_data->filename, '\0', sizeof(client_data->filename));
      strncpy(client_data->filename, buffer, strlen(buffer));
      buffer = strtok(NULL, "/\n");
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

#if 0
void Send_Chunk(void *filename, uint8_t hash_mod_value, void *data_chunk,size_t chunk_size,\
                          uint8_t chunk_seq, client_config_data_t *client_data)
{
  if(data_chunk == NULL) {
    return;
  }
  
  FILE *fp = NULL;
  char write_string[70];
  int i = 0;
  struct stat st = {0};

  /* this loop runs for as many duplicate chunks that are require */
  for(i = 0; i<CHUNK_DUPLICATES; i++) {    
   
    /* The below logic checks if the server is authenticated or not */
    if(!auth_server_list[(Distribution_Schema[hash_mod_value][chunk_seq][i] - 1)])
      continue;
    
    memset(write_string, '\0', strlen(write_string));
    sprintf(write_string, "../DFS_Server/DFS%d/%s",\
        Distribution_Schema[hash_mod_value][chunk_seq][i], client_data->username);
    printf("Sending... hash_val: %d\nchunk %d\npath: %s\n", hash_mod_value, chunk_seq, write_string);
    if(stat(write_string, &st) == -1) {
      printf("%s directory does not exist, create now!\n", client_data->username);
      int ret_val = mkdir(write_string, 0777);
      if(ret_val < 0) {
        perror("");
        return;
      }
    }
    sprintf(write_string, "../DFS_Server/DFS%d/%s/.%s.chunk_%d",\
        Distribution_Schema[hash_mod_value][chunk_seq][i], client_data->username,\
                                client_data->filename, chunk_seq);  
    fp = fopen(write_string, "wb");
    if(fp == NULL) {
      printf("<%s>: Unable to open the file!\n", __func__);
      return;
    }
    //fwrite(local_chunk_storage, sizeof(char), strlen(local_chunk_storage), fp);
    fwrite((char *)data_chunk, 1, chunk_size, fp);
    fclose(fp);
  }

  return;
}
#endif

void Send_Chunk(uint8_t *sock_list, void *filename, uint8_t hash_mod_value, \
                  void *data_chunk,size_t chunk_size,\
                  uint8_t chunk_seq, client_config_data_t *client_data)
{

  /* Do a preliminary check and open the file that has to be sent */
  if(data_chunk == NULL) {
    return;
  }
  
  FILE *fp = NULL;
  char write_string[70];
  int i = 0;
  char sync_command[5];
  memset(sync_command, '\0', sizeof(sync_command));

  char xor_array[chunk_size];
  memset(xor_array, '1', sizeof(xor_array));

  /* this loop runs for as many duplicate chunks that are require */
  for(i = 0; i<CHUNK_DUPLICATES; i++) {    
  
    memset(sync_command, '\0', sizeof(sync_command));

    /* The below logic checks if the server is authenticated or not */
    if(!auth_server_list[(Distribution_Schema[hash_mod_value][chunk_seq][i] - 1)])
      continue;

    printf("Sending chunk - %d\n",\
        (Distribution_Schema[hash_mod_value][chunk_seq][i] - 1));
    
    /* Send the size of the chunk first */
    send(sock_list[(Distribution_Schema[hash_mod_value][chunk_seq][i] - 1)],\
           &chunk_size, sizeof(size_t), 0);

    /* Send this data chunk to the correct server as per the schema */
#ifdef ENCRYPTION_ENABLED
    memset(xor_array, '1', sizeof(xor_array));
    for(int i = 0; i<sizeof(xor_array); i++) {
      xor_array[i] = xor_array[i] ^ ((char *)data_chunk)[i];
    }
    send(sock_list[(Distribution_Schema[hash_mod_value][chunk_seq][i] - 1)],\
        xor_array, chunk_size, 0);
#else
    send(sock_list[(Distribution_Schema[hash_mod_value][chunk_seq][i] - 1)],\
        data_chunk, chunk_size, 0);
#endif

    /* Next send the chunk number as well */
    send(sock_list[(Distribution_Schema[hash_mod_value][chunk_seq][i] - 1)],\
          &chunk_seq, sizeof(uint8_t), 0);
 
    /* Send the server number as well */
    send(sock_list[(Distribution_Schema[hash_mod_value][chunk_seq][i] - 1)],\
          &Distribution_Schema[hash_mod_value][chunk_seq][i], sizeof(uint8_t), 0);

    /* Send the name of the file as well */
    size_t file_name_len = strlen(client_data->filename);
    printf("file_name_len: %lu\n", file_name_len);
    send(sock_list[(Distribution_Schema[hash_mod_value][chunk_seq][i] - 1)],\
        &file_name_len, sizeof(size_t), 0); 
    send(sock_list[(Distribution_Schema[hash_mod_value][chunk_seq][i] - 1)],\
        client_data->filename, file_name_len, 0);
  
    /* To avoid any sync. issues, wait for a message */
    recv(sock_list[(Distribution_Schema[hash_mod_value][chunk_seq][i] - 1)],\
            sync_command, sizeof(sync_command), 0);
  }
  
  return;
}

infra_return Auth_Client_Connections(int *return_accept_socket, server_config_data_t *server_config)
{
  /* 1. Get the username and the password from the client 
   * 2. Check in the struct array if the username and pass 
   * are valid or not
   */
  char buffer[50];
  memset(buffer, '\0', sizeof(buffer));

  static uint8_t server_count = 0;

  recv(*return_accept_socket, buffer, 50, 0);
  infra_return ret_val = Validate_Login_Credentials(buffer, server_config, &server_count);
  if(ret_val == AUTH_FAILURE) {
    printf("Authentication failed!; Disconnecting Server!\n");
    send(*return_accept_socket, "fail", strlen("fail"), 0);
    return AUTH_FAILURE; 
  }
  
  send(*return_accept_socket, "pass", strlen("pass"), 0);

  return AUTH_SUCCESS;
}

infra_return Accept_Auth_Client_Connections(int *return_accept_socket, int server_socket,\
                                  struct sockaddr_in *address, socklen_t addr_len,\
                                      server_config_data_t *server_config)
{
  /* Accept the connection */
  *return_accept_socket = accept(server_socket, (struct sockaddr *)address, &addr_len);

  infra_return return_value = Auth_Client_Connections(return_accept_socket, server_config);

  return return_value;
}

void Chunk_Store_File(uint8_t *sock_list, void *filename, uint8_t hash_mod_value,\
                          client_config_data_t *client_data)
{
  FILE *fp = fopen(filename, "rb");
  if(fp == NULL) {
    printf("file %s can't be opened!\n", (char *)filename);
    return;
  }

  /* 1. Get the size of the file first and alloc an array accordingly */
  uint32_t file_size = 0;
  fseek(fp, 0L, SEEK_END);
  file_size = ftell(fp);
  rewind(fp);

  uint8_t seq = 0;
  uint32_t read_count = 0;

  /* If the file size is not a multiple of 4, stuff the remaining 
   * bytes into the last file chunk
   */
  uint32_t packet_size = (file_size/4);
 
  printf("File Size: %d packet_size: %d\n", file_size, packet_size);

  char chunk_storage[10000];
  memset(chunk_storage, '\0', sizeof(chunk_storage));

  while(!feof(fp)) {
    /* Buffer Cleanup */ 
    memset(chunk_storage, '\0', sizeof(chunk_storage));
   
    /* Need to handle the case for the last packet */
    if(seq == (MAX_DFS_SERVERS-1)) {
      printf("Last packet NOW!\n");
      read_count = fread(chunk_storage, 1, (packet_size + (file_size%4)), fp);

      Send_Chunk(sock_list, filename, hash_mod_value, chunk_storage, read_count, seq, client_data); 
      return;
    } else {
      read_count = fread(chunk_storage, 1, packet_size, fp);
      printf("<<%s>>: read_count: %d\n", __func__, read_count);
    } 

    /* Send the chunk to its appropriate location */
    Send_Chunk(sock_list, filename, hash_mod_value, chunk_storage, read_count, seq, client_data); 
    seq++;
  }

  fclose(fp);

  return;
}

void Chunk_File(void *filename)
{
  FILE *fp = fopen(filename, "rb");
  if(fp == NULL) {
    printf("file %s can't be opened!\n", (char *)filename);
    return;
  }
  
  /* 1. Get the size of the file first and alloc an array accordingly */
  uint32_t file_size = 0;
  fseek(fp, 0L, SEEK_END);
  file_size = ftell(fp);
  rewind(fp);

  /* If the file size is not a multiple of 4, stuff the remaining 
   * bytes into the last file chunk
   */
  uint32_t packet_size = (file_size/4);
 
#ifdef DEBUG_GENERAL
  printf("PacketSize: %d\n", packet_size);
  printf("File Size: %d div/4: %d\n", file_size, file_size/4);
#endif

  int i = 0;
  int cnt = 0;
  char fp_read_char = '\0';

  uint8_t file_counter = 1;
  FILE *output_file = NULL;
  
  memset(chunk_filename_list, '\0', sizeof(chunk_filename_list));

  sprintf(chunk_filename_list[file_counter-1], "chunk_%d", file_counter);
  output_file = fopen(chunk_filename_list[file_counter-1], "w");

  while((cnt = fgetc(fp)) != EOF) {
    fp_read_char = (char)cnt;  
    fputc(fp_read_char, output_file);
    i++;
    
    if(i == packet_size && file_counter < 4) {
      
      fclose(output_file);
      file_counter++; i = 0;

      sprintf(chunk_filename_list[file_counter-1], "chunk_%d", file_counter);
      output_file = fopen(chunk_filename_list[file_counter-1], "w");
      continue;
    } 
  }

  fclose(output_file);
  fclose(fp);
  return;
}

void create_md5_hash(char *digest_buffer, char *buffer, int buffer_size)
{
  char digest[16];
  MD5_CTX md5_struct;

  MD5_Init(&md5_struct);
  MD5_Update(&md5_struct, buffer, buffer_size);
  MD5_Final(digest, &md5_struct);

  return;
}

uint8_t Generate_MD5_Hash(void *filename, unsigned char *ret_digest)
{
  FILE *fp = fopen(filename, "rb");
  if(fp == NULL) {
    printf("file %s can't be opened!\n", (char *)filename);
    return 0;
  }

  /* Initialize the openssl MD5 structs */
  MD5_CTX md5context;
  int bytes = 0;
  unsigned char data[1024];
  memset(data, '\0', sizeof(data));
  
  unsigned int digest_buffer[MD5_DIGEST_LENGTH];
  memset(&digest_buffer, 0, sizeof(digest_buffer));

  /* Initialize the MD5 and for every 1027 bytes, update the MD5 digest */
  MD5_Init(&md5context);
  while((bytes = fread(data, 1, 1024, fp)) != 0)
    MD5_Update(&md5context, data, bytes);

  /* Create a 128bit MD5 number and store in ret_digest array */
  MD5_Final(ret_digest, &md5context);
 
  char temp_store[5];
  memset(temp_store, '\0', sizeof(temp_store));
  uint32_t hash_hex_value = 0;
  uint8_t temp = 0;

  /* Iterate over the 16B for the MD5 digest and convert to a long num.
   * next, shift the byte into a 32bit number 
   * At, the end, we return the mod 4 value of this 32bit number 
   */
  for(int i = 0; i<MD5_DIGEST_LENGTH; i++) {
    
    sprintf(temp_store, "%x", ret_digest[i]);
    temp = strtol(temp_store, NULL, 16);
    
    hash_hex_value = hash_hex_value | (temp << ((MD5_DIGEST_LENGTH/2) * (3-i)));
    printf("\n");
    if((3 - i) == 0)
      break;
  }

  fclose(fp);

  return (hash_hex_value%4);
}


void Create_Client_Connections(int8_t *client_socket, uint16_t port_number,\
                                 struct sockaddr_in *server_addr, size_t server_addr_len)
{
  *client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(*client_socket < 0) {
    perror("ERROR:socket()\n");
    return;
  }

  printf("port_num: %d\n",port_number);
  server_addr->sin_family = AF_INET;
  server_addr->sin_port = htons(port_number);
  server_addr->sin_addr.s_addr = INADDR_ANY;
  memset(server_addr->sin_zero, '\0', sizeof(server_addr->sin_zero));

  /* Connect to the IP on the port */
  connect(*client_socket, (struct sockaddr *)server_addr, server_addr_len);

  return;
}

static uint8_t username_cntr = 0;
static uint8_t password_cntr = 0;

void Parse_Server_Config_File(void *filename, server_config_data_t *config)
{
  size_t read_line = 0;
  char *line = (char *)malloc(50);
  FILE *fp_config_file = NULL;
  char *buffer = NULL;
  uint8_t cntr = 0;

  fp_config_file = fopen((char *)filename, "rb");
  if(fp_config_file == NULL) {
    fprintf(stderr, "%s: ", (char *)filename);
    perror("");
    exit(0);
  }

  /* Clear the struct buffers */
  memset(config->username, '\0', sizeof(config->username));
  memset(config->password, '\0', sizeof(config->password));

  while( (read_line = getline(&line, &read_line, fp_config_file)) != -1) {
    cntr = 0;
    buffer = strtok(line, " \n");
    while(buffer != NULL) {
      switch(cntr)
      {
        case 0: /* Get the username and store it */
          memcpy(config->username[username_cntr], buffer, strlen(buffer));
          username_cntr++;
          break;

        case 1: /* Get the password and store it */
          memcpy(config->password[password_cntr], buffer, strlen(buffer));
          password_cntr++;
          break;

        default: /* Do Nothing */
          break;
      }
      buffer = strtok(NULL, " \n");
      cntr++;
    }
  }

#if 0 
  for(int i = 0; i<2; i++) {
    printf("config->username: %s\n", config->username[i]);
    printf("config->password: %s\n", config->password[i]);
  }
#endif

  free(line);
  return;
}

void Create_Server_Connections(int *server_sock, struct sockaddr_in *server_addr,\
                                int server_addr_len, int tcp_port)
{
  if(server_addr == NULL) {
    return;
  }

  (*server_sock) = socket(AF_INET, TCP_SOCKETS, 0);

  /* enable socket re-use */
  int enable = 1;
  if(setsockopt((*server_sock), SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed!!\n");


  /* set the parameters for the server */
  server_addr->sin_family = AF_INET;
  server_addr->sin_port = htons(tcp_port);
  server_addr->sin_addr.s_addr = INADDR_ANY;

  /* Bind the server to the client */
  int ret_val = bind(*server_sock, (struct sockaddr *)server_addr, server_addr_len);
  if(ret_val < 0) {
    perror("ERROR:bind()\n");
    close(*server_sock); 
    exit(1);
  }

  /* Start listening for the connection on the socket 
   * Currently, limit the backlog connections to 5. This value will be 
   * maxed out depending on the value that is in /proc/sys/ipv4/tcp_max_syn_backlog
  */
  ret_val = listen(*server_sock, 5);
  if(ret_val < 0) {
    perror("ERROR:listen()\n");
    exit(1);
  }
  
  return;
}

void Dump_Client_Data(client_config_data_t *config, uint8_t net_addresses)
{
  if(config == NULL)
    return;

  for(int i = 0; i<net_addresses; i++) {
    printf("DFS_Server_Names: %s\n", config->DFS_Server_Names[i]);
    printf("IP Address      : %s\n", config->client_ports.IP_Addr[i]);
    printf("Port Num        : %d\n", config->client_ports.port_num[i]);
    printf("Username        : %s\n", config->username);
    printf("Password        : %s\n", config->password);
    printf("---------------------------------\n");
  }

  return;
}

void Parse_Client_Config_File(void *filename, client_config_data_t *config)
{
  size_t read_line = 0;
  uint8_t cntr_words = 0;
  char *line = (char *)malloc(50);
  FILE *fp_config_file = NULL;
  
  fp_config_file = fopen((char *)filename, "rb");
  if(fp_config_file == NULL) {
    fprintf(stderr, "%s: ", (char *)filename);
    perror("");
    exit(0);
  }

  /* Clear the buffers before proceeding */
  memset(config->DFS_Server_Names, '\0', sizeof(config->DFS_Server_Names));
  memset(config->client_ports.IP_Addr, '\0', sizeof(config->client_ports.IP_Addr));
  memset(config->client_ports.port_num, 0, sizeof(config->client_ports.port_num));
  memset(config->username, '\0', sizeof(config->username));
  memset(config->password, '\0', sizeof(config->password));


  while( (read_line = getline(&line, &read_line, fp_config_file)) != -1) {
    
    /* Store the value of the first word of every line */
    char temp_name[10];
    memset(temp_name, '\0', sizeof(temp_name));
    for(int i = 0; i<8; i++) {
      temp_name[i] = line[i];
    }
   
    /* Get the username from the file */
    if(!strcmp(temp_name, "Username")) {
      uint8_t i = 0;
      char *buf = strtok(line, " \n");
      while(buf != NULL) {
        if(i == 1) {
          strcpy(config->username, buf);
        }
        i++;
        buf = strtok(NULL, " \n");
      }
      continue;
    }

    /* Get the password from the file */
    if(!strcmp(temp_name, "Password")) {
      uint8_t i = 0;
      char *buf = strtok(line, " ");
      while(buf != NULL) {
        if(i == 1) {
          strcpy(config->password, buf);
        }
        i++;
        buf = strtok(NULL, "\n");
      }
      continue;
    }

    /* get the server name, IP and Port number from the extracted line */ 
    cntr_words = 0;
    char *buf = NULL;
    char *buffer = strtok(line, " \n");
    uint8_t gen_counter = 0;
    static uint8_t cnt_server_name = 0;
    static uint8_t cnt_ip_port = 0;
    
    while(buffer != NULL) {
      switch(cntr_words)
      {
        case 0:
          /* This is just the word "Server"; do nothing */
          break;

        case 1:
          /* This is the DFS server name.
           * Store the names of the servers in memory 
           */
          memcpy(config->DFS_Server_Names[cnt_server_name], buffer, strlen(buffer));
          cnt_server_name++;
          break;

        case 2:
          /* This is the IP address and the port number */
          buf = strtok(buffer, ":");
          while(buf != NULL) {
            switch(gen_counter)
            {
              case 0: /* This is the IP address */
                      strcpy(config->client_ports.IP_Addr[cnt_ip_port], buffer);
                      break;
              case 1: /* This is the port number */
                      config->client_ports.port_num[cnt_ip_port] = atoi(buf);
                      break;
              default: /*Do Nothing*/
                        printf(" You shouldn't be here!\n");
                        break;
            }
            gen_counter++;
            buf = strtok(NULL, "\n");
          }
          cnt_ip_port++;
          break;

        default:
          /* Nothing to do here */
          printf("default   %s\n", buffer);
          break;
      }
      buffer = strtok(NULL, " \n");
      cntr_words++;
    }
  }

  fclose(fp_config_file);
  free(line);

  return;
}

void Execute_Put_File(uint8_t *sock_list, void *filename, client_config_data_t *client_data)
{
  /* Get the hash of the file */
  unsigned char digest_buffer[MD5_DIGEST_LENGTH];
  memset(digest_buffer, '\0', sizeof(digest_buffer));
    
  memcpy(cache_put_filenames[put_file_count++], client_data->filename,\
                    strlen(client_data->filename));
  printf("filename: %s\n", cache_put_filenames[put_file_count-1]);

  uint8_t hash_mod_val = Generate_MD5_Hash(filename, digest_buffer); 
  printf("<<%s>>: hash_mod_val: %d\n", __func__, hash_mod_val);
  
  /* Send the filename */
  printf("filename:%s\n", client_data->filename); 
  
  Chunk_Store_File(sock_list, filename, hash_mod_val, client_data);
  
  return;
}

void Execute_List(client_config_data_t *client_data)
{
  /* Query all the servers and get the files from them */
  DIR *directory = NULL;
  struct dirent *dir = NULL;

  /* Contruct the path to the directories and check if they are available */
  char path_string[60];
  memset(path_string, '\0', sizeof(path_string));
  
  uint8_t chunk_checklist[MAX_DFS_SERVERS];
  memset(chunk_checklist, 0, sizeof(chunk_checklist));

  /* File Look Up Counter*/
  /* The first index is the server number and the second index
   * denotes the chunk number. This let's you know that chunk 
   * number is there in which server
   */
  uint8_t local_file_lookup[MAX_DFS_SERVERS][MAX_DFS_SERVERS];
  memset(local_file_lookup, 0, sizeof(local_file_lookup));

  char file_prefix[40];
  memset(file_prefix, '\0', sizeof(file_prefix));
 
  /* TODO: This needs to be abstracted further. It's utterly unreadable and 
   *        I can't maintain it like this!
   */

  for(int file_count = 0; file_count < put_file_count; file_count++) {
  
    memset(chunk_checklist, 0, sizeof(chunk_checklist));
    memset(local_file_lookup, 0, sizeof(local_file_lookup));
    memset(file_prefix, '\0', sizeof(file_prefix));
    
    for(int i = 0; i<MAX_DFS_SERVERS; i++) {
      if(auth_server_list[i]) {
        memset(path_string, '\0', sizeof(path_string));
        sprintf(path_string, "../DFS_Server/DFS%d/%s", (i+1), client_data->username);

        directory = opendir(path_string);
        if(directory) {
          while((dir = readdir(directory)) != NULL) {

            /* Discard these 2 cases! */
            if(!strcmp(dir->d_name, ".") || !(strcmp(dir->d_name, "..")))
              continue;

            /* Query through the name list to see if you find a match */
            for(int j = 0; j<MAX_DFS_SERVERS; j++) {
                sprintf(file_prefix, ".%s%s", cache_put_filenames[file_count],\
                  chunk_names[j]);
              if(!strcmp(dir->d_name, file_prefix)) {
                /* If you find a match, just increment the counter for 
                 * that particular chunk number. That way you know what 
                 * is there in what server.
                 */     
                (local_file_lookup[i][j])++;
                chunk_checklist[j]++;
                break;
              }
            }
            }
          }
          closedir(directory);
        }
      }
      printf("FILE: %s\n", cache_put_filenames[file_count]);
      for(int i = 0; i<MAX_DFS_SERVERS; i++) {
        if(chunk_checklist[i] < 1) {
          printf("%s incomplete!\n", chunk_names[i]);
        } else {
          printf("%s\n", chunk_names[i]);
        }
      }
    }

#ifdef DEBUG_GENERAL
  for(int j = 0; j<MAX_DFS_SERVERS; j++) {
    printf("local_file_lookup[%d]: %d %d %d %d\n",j,\
        local_file_lookup[j][0], local_file_lookup[j][1],\
        local_file_lookup[j][2], local_file_lookup[j][3]);
  }
#endif

  return;
}

#if 0
void Get_File_From_Servers(client_config_data_t *client_data)
{
  DIR *directory = NULL;
  struct dirent *dir = NULL;
  
  /* Query the server locations for the chunks; in order */
  char path_string[60];
  memset(path_string, '\0', sizeof(path_string));
 
  bool found = false;

  for(int j = 0; j<MAX_DFS_SERVERS; j++) {
    for(int i = 0; i<MAX_DFS_SERVERS; i++) {
      memset(path_string, '\0', sizeof(path_string));
      sprintf(path_string, "../DFS_Server/DFS%d/%s", (i+1), client_data->username);
      //printf("%s\n", path_string);

      directory = opendir(path_string);
      if(directory) {
        while((dir = readdir(directory)) != NULL) {

          /* Discard these 2 cases! */
          if(!strcmp(dir->d_name, ".") || !(strcmp(dir->d_name, "..")))
            continue;
         
        
          if(!strcmp(dir->d_name, chunk_names[j])) {
            /* Do the read and write operation now! */
            printf("%s\n", dir->d_name);
            found = true;
            break;
          }
        }
      } else {
        printf("Unable to open the file!\n");
      }
      if(found == true) {
        closedir(directory);
        found = false;
        break;
      } else {
        closedir(directory);
      }
    }
  }
  return;
}
#endif    


void Authenticate_Client_Connections(int8_t *client_socket,\
                client_config_data_t *client_data, struct sockaddr_in *server_addr)
{
  
  for(int i = 0; i<MAX_DFS_SERVERS; i++) {
    Create_Client_Connections(&client_socket[i], client_data->client_ports.port_num[i],\
        &server_addr[i], sizeof(server_addr[i])); 
    infra_return ret = Send_Auth_Client_Login(client_socket[i], client_data);
    if(ret == AUTH_SUCCESS)
      auth_server_list[i]++;

    send(client_socket[i], global_client_buffer[0], strlen(global_client_buffer[0]), MSG_NOSIGNAL);

  }

  return;
}

void Execute_Put_Server(int *accept_ret, server_config_data_t *server_config)
{
  char data_buffer[10000];
  memset(data_buffer, '\0', sizeof(data_buffer));

  char write_string[70];
  memset(write_string, '\0', sizeof(write_string));

  size_t filename_len = 0;
  char filename[20];
  memset(filename, '\0', sizeof(filename));
  
  struct stat st = {0};
  uint8_t chunk_seq_num = 0;
  uint8_t server_num = 0;
  size_t chunk_size = 0;


  for(int i = 0; i<CHUNK_DUPLICATES; i++) {    
    /* Get the size of the chunk */
    recv(*accept_ret, &chunk_size, sizeof(size_t), 0);
    
    /* receive the chunk packet */
    recv(*accept_ret, data_buffer, chunk_size, 0); 

    /* receive the chunk seq number as well */
    recv(*accept_ret, &chunk_seq_num, sizeof(uint8_t), 0);
  
    /* receive the dfs server num */
    recv(*accept_ret, &server_num, sizeof(uint8_t), 0);
  
    /* receive the name of the file */
    recv(*accept_ret, &filename_len, sizeof(size_t), 0); 
    recv(*accept_ret, filename, filename_len, 0);
    
    /* Store filename */
    memcpy(server_config->filename[0], filename, strlen(filename));

    sprintf(write_string, "./DFS%d/%s/", server_num, server_config->username[0]);
    /* check if a dir for the user exists or not */
    if(stat(write_string, &st) == -1) {
      printf("%s directory does not exist, create now!\n", server_config->username[0]);
      int ret_val = mkdir(write_string, 0777);
      if(ret_val < 0) {
        perror("");
        return;
      }
    }
    
    /* Create the file and write the contents to it */ 
    sprintf(write_string, "./DFS%d/%s/.%s.chunk_%d", server_num,\
        server_config->username[0], filename, chunk_seq_num);
    
    FILE *fp = NULL;
    fp = fopen(write_string, "wb");
    if(fp == NULL) {
      printf("<%s>: Unable to open the file!\n", __func__);
      return;
    }

    fwrite(data_buffer, 1, chunk_size, fp);
    fclose(fp);

    send(*accept_ret, "cont", strlen("cont"), 0); 

#ifdef DEBUG_ALL
    printf("server_num: %d\n", server_num);
    printf("chunk_size: %lu\n", chunk_size);
    //printf("%s\n", data_buffer);
    printf("chunk_seq_num: %d\n", chunk_seq_num);
#endif

  }

  return;
}

void removeSubstring(char *s,const char *toremove)
{
    while( s=strstr(s,toremove) )
          memmove(s,s+strlen(toremove),1+strlen(s+strlen(toremove)));
}

void Execute_List_Server(int *accept_ret, char *server_name,\
                            server_config_data_t *server_config)
{
  /* Open the dir as per the username and peek */
  DIR *directory = NULL;
  struct dirent *dir = NULL;

  /* Contruct the path to the directories and check if they are available */
  char path_string[60];
  memset(path_string, '\0', sizeof(path_string));

  char temp_d_name[30];
  memset(temp_d_name, '\0', sizeof(temp_d_name));
  
  char removal_string[30];
  memset(removal_string, '\0', sizeof(removal_string));
  
  sprintf(removal_string, ".%s.", server_config->filename[0]);
  printf("removal_string: %s\n", removal_string);

  sprintf(path_string, "./%s/%s", server_name, server_config->username[0]);

  directory = opendir(path_string);
  if(directory) {
    while((dir = readdir(directory)) != NULL) {

      /* Discard these 2 cases! */
      if(!strcmp(dir->d_name, ".") || !(strcmp(dir->d_name, "..")))
        continue;
      
      memset(temp_d_name, '\0', sizeof(temp_d_name));
      memcpy(temp_d_name, dir->d_name, strlen(dir->d_name));
      //printf("%s\n", temp_d_name);
  
      removeSubstring(temp_d_name, removal_string);
      printf("Post Removal: %s\n", temp_d_name);
     
      /* Send this over to the client for it to parse */
      int send_ret = send(*accept_ret, temp_d_name, strlen(temp_d_name), MSG_NOSIGNAL);
      if(send_ret < 0) {
        perror("");
        return;
      }
    }
  }
 
  return;
}


void Execute_List_Client(int8_t *client_socket, client_config_data_t *client_data)
{
  int recv_ret = 0;
  /* Receive data from the servers */
  char chunk_buffer[8];
  memset(chunk_buffer, '\0', sizeof(chunk_buffer));
  
  uint8_t chunk_checklist[MAX_DFS_SERVERS];
  memset(chunk_checklist, 0, sizeof(chunk_checklist));
  
  uint8_t local_file_lookup[MAX_DFS_SERVERS][MAX_DFS_SERVERS];
  memset(local_file_lookup, 0, sizeof(local_file_lookup));
  
  for(int j = 0; j<MAX_DFS_SERVERS; j++) {
    
    for(int i = 0 ; i<2; i++) {
      memset(chunk_buffer, '\0', sizeof(chunk_buffer));
      recv_ret = recv(client_socket[j], chunk_buffer, 7, 0);
      if(recv_ret < 0) {
        perror("");
        return;
      }
      printf("%s\n", chunk_buffer);

      for(int k = 0; k<MAX_DFS_SERVERS; k++) {
        if(!strcmp(chunk_buffer, chunk_names[k])) {
          local_file_lookup[j][k]++;
          chunk_checklist[k]++;
        }
      } 
    }
  }
#if 1 
  printf("Chunk Storage -->\n");
  for(int i = 0; i<MAX_DFS_SERVERS; i++) {
    for(int k = 0; k<MAX_DFS_SERVERS; k++) {
      printf("%d ", local_file_lookup[i][k]);
    }
    printf("\n");
  }
#endif
  
  for(int i = 0; i<MAX_DFS_SERVERS; i++) {
    if(chunk_checklist[i] < 1) {
      printf("%s incomplete!\n", chunk_names[i]);
    } else {
      printf("%s\n", chunk_names[i]);
    }
  }

  return;
}





