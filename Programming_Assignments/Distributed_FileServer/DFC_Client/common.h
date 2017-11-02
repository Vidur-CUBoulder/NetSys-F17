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


typedef struct __client_IP_and_Port_t {
  char IP_Addr[10][10];
  uint16_t port_num[10];
} client_ip_port_t;

typedef struct __client_config_data_t {
  char DFS_Server_Names[10][10];
  client_ip_port_t client_ports;
  char username[10];
  char password[10];
} client_config_data_t;

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


