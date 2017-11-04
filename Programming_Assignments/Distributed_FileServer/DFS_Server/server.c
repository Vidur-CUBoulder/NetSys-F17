#include "../common/common.h"

int main(int argc, char *argv[])
{
  switch(argc)
  {
    case 1: /* Config. file not passed */
            printf("Configuration file not passed!\n");
            exit(0);
            break;

    case 2: /* This is the Server name */
            printf("Server name not passed!\n");
              exit(0);
              break;

    case 3: /* This is the port number */
            printf("Port number not passed!\n");
            exit(0);
            break;

    default:/* Do nothing in this case */
            break;
  }

  /* Sanitize the DFS server names passed! */
  if(strcmp(argv[2], "DFS1") && strcmp(argv[2], "DFS2") &&\
      strcmp(argv[2],"DFS3") && strcmp(argv[2], "DFS4")) {
    printf("Invalid DFS Server name!\n");
  }

  /* Parse the config file and store its contents */
  server_config_data_t server_config;
  Parse_Server_Config_File(argv[1], &server_config);

  /* Get the port number and setup the server */
  uint16_t port_num = atoi((argv[3]));
  int server_socket = 0;

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(server_socket < 0) {
    perror("ERROR:socket()\n");
    return 0;
  }

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
 
  /* Setup the webserver connections */
  Create_Server_Connections(&server_socket, &address,\
                    sizeof(address), port_num);

  /* Accept the connection */
  socklen_t addr_len = sizeof(address);
  printf("here!!\n");
  int accept_ret = accept(server_socket, (struct sockaddr *)&address, &addr_len);

#ifdef TEST_SERVER_CONNECTIONS 
  char buffer[20];
  memset(buffer, '\0', sizeof(buffer));
  strcpy(buffer, "hello world");

  /* Server send test */
  int send_ret = send(accept_ret, buffer, strlen(buffer), 0);
  if(send_ret < 0) {
    perror("SEND");
    exit(0);
  }
#endif

  /* Server receive test */
  char buffer[48];
  memset(buffer, '\0', sizeof(buffer));
  recv(accept_ret, buffer, 48, 0);
  printf("buffer: %s\n", buffer);

  return 0;
}

