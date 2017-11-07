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

#define TCP_SOCKETS SOCK_STREAM

#define MAX_DFS_SERVERS 4

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
  char username[15];
  char password[15];
} client_config_data_t;

typedef struct __server_config_data_t {
  char username[5][15];
  char password[5][15];
} server_config_data_t;

char chunk_filename_list[4][20];

char global_client_buffer[2][20];

char *valid_commands[] = {  
  "put", 
  "get",
  "list",
  "exit",
  "clear"
};

infra_return Validate_Login_Credentials(char *input_buffer,\
                            server_config_data_t *config_data)
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
        return AUTH_SUCCESS;
      }
    }
    local_buffer = strtok(NULL, " \n");
  }
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


/* Eergghh!! The dirtiest function ever written!! */
void Distribute_Chunks(uint8_t hash_mod_value)
{
  char system_string[50];
  memset(system_string, '\0', sizeof(system_string));
  switch(hash_mod_value)
  {
    default:/* hash % 4 == 0 */
      printf("Case 0\n");
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[0],\
          "../DFS_Server/DFS1");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[1],\
          "../DFS_Server/DFS1");
      system(system_string);

      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[1],\
          "../DFS_Server/DFS2");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[2],\
          "../DFS_Server/DFS2");
      system(system_string);

      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[2],\
          "../DFS_Server/DFS3");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[3],\
          "../DFS_Server/DFS3");
      system(system_string);

      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[3],\
          "../DFS_Server/DFS4");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[0],\
          "../DFS_Server/DFS4");
      system(system_string);

      break;

    case 1: /* hash % 4 == 1 */
      printf("Case 1\n");
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[3],\
          "../DFS_Server/DFS1");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[0],\
          "../DFS_Server/DFS1");
      system(system_string);

      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[0],\
          "../DFS_Server/DFS2");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[1],\
          "../DFS_Server/DFS2");
      system(system_string);

      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[1],\
          "../DFS_Server/DFS3");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[2],\
          "../DFS_Server/DFS3");
      system(system_string);

      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[2],\
          "../DFS_Server/DFS4");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[3],\
          "../DFS_Server/DFS4");
      system(system_string);

      break;

    case 2: /*hash % 4 == 2 */
      printf("Case 2\n");
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[2],\
          "../DFS_Server/DFS1");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[3],\
          "../DFS_Server/DFS1");
      system(system_string);

      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[3],\
          "../DFS_Server/DFS2");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[0],\
          "../DFS_Server/DFS2");
      system(system_string);

      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[0],\
          "../DFS_Server/DFS3");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[1],\
          "../DFS_Server/DFS3");
      system(system_string);

      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[1],\
          "../DFS_Server/DFS4");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[2],\
          "../DFS_Server/DFS4");
      system(system_string);

      break;

    case 3: /* hash % 4 == 3 */
      printf("Case 3\n");
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[1],\
          "../DFS_Server/DFS1");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[2],\
          "../DFS_Server/DFS1");
      system(system_string);

      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[2],\
          "../DFS_Server/DFS2");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[3],\
          "../DFS_Server/DFS2");
      system(system_string);

      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[3],\
          "../DFS_Server/DFS3");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[0],\
          "../DFS_Server/DFS3");
      system(system_string);

      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[0],\
          "../DFS_Server/DFS4");
      system(system_string);
      sprintf(system_string, "cp -rvf %s %s", chunk_filename_list[1],\
          "../DFS_Server/DFS4");
      system(system_string);

      break;
  }

  /* Cleanup in the present dir. */
  system("rm -rvf chunk_*");

  return;
}



void Chunk_File(void *filename, uint8_t chunk_option)
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
  
  MD5_CTX md5context;
  int bytes = 0;
  unsigned char data[1024];
  memset(data, '\0', sizeof(data));
  
  unsigned int digest_buffer[MD5_DIGEST_LENGTH];
  memset(&digest_buffer, 0, sizeof(digest_buffer));

  MD5_Init(&md5context);
  while((bytes = fread(data, 1, 1024, fp)) != 0)
    MD5_Update(&md5context, data, bytes);

  MD5_Final(ret_digest, &md5context);
 
  char temp_store[5];
  memset(temp_store, '\0', sizeof(temp_store));
  uint32_t hash_hex_value = 0;
  uint8_t temp = 0;

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


void Create_Client_Connections(uint8_t *client_socket, uint16_t port_number,\
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


void Parse_Server_Config_File(void *filename, server_config_data_t *config)
{
  size_t read_line = 0;
  char *line = (char *)malloc(50);
  FILE *fp_config_file = NULL;
  char *buffer = NULL;
  uint8_t cntr = 0;

  static uint8_t username_cntr = 0;
  static uint8_t password_cntr = 0;

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

void Execute_Put_File(void *filename)
{
  /* Get the hash of the file */
  unsigned char digest_buffer[MD5_DIGEST_LENGTH];
  memset(digest_buffer, '\0', sizeof(digest_buffer));
  
  //uint8_t hash_mod_val = Generate_MD5_Hash("text1.txt", digest_buffer); 
  uint8_t hash_mod_val = Generate_MD5_Hash(filename, digest_buffer); 

  /* Create the file chunks */
  Chunk_File(filename, hash_mod_val);
 
#ifdef DEBUG_GENERAL
  printf("Filename List\n");
  for(int i = 0; i<4; i++) {
    printf("%s\n", chunk_filename_list[i]);
  }
#endif

  /* Distribute the file chunks to the appropriate servers */
  Distribute_Chunks(hash_mod_val);

  return;
}



