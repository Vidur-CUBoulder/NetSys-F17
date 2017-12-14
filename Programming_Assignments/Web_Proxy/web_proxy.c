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
#include<fcntl.h>
#include<sys/time.h>
#include<openssl/md5.h>
#include<time.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/stat.h>
#include<dirent.h>

#define MAX_CONNECTIONS 10000
#define TCP_SOCKETS SOCK_STREAM
#define CACHE_DIR "cache"
#define DEBUG_URL_PARSER
#define MAX_BUFFER_SIZE 2048

int32_t client_slots[MAX_CONNECTIONS];

char respond_403_error[200] = "<html><body>403 FORBIDDEN</body></html>\0";
char respond_400_error[200] = "<html><body>400 Bad Request Reason: Invalid Method</body></html>\0";
char respond_404_error[300] = "<html><body>404 NOT Found Reason URL does not exist</body></html>\0";

uint16_t proxy_timeout_value = 0;

char blacklist_array[10][80];
uint8_t blacklist_count = 0;

struct hostent cache_he[10];

typedef enum __blacklist_status_t {
  ACCEPT, 
  REJECT
} blacklist_status;

typedef struct __URL_information {
  uint32_t port_num;
  char ip_addr[20];
  char client_command[10];
  char domain_name[100];
  char filename[1000];
  char http_version[20];
  char domain_name_hash[(MD5_DIGEST_LENGTH*2)+1];
  bool cache_file;
} parsed_url;

typedef struct __cache_file_operations {
  char pwd_string[100];
} cache_info;

void create_server_connections(int *server_sock, struct sockaddr_in *server_addr,\
                                int server_addr_len, int tcp_port)
{
  if(server_addr == NULL) {
    return;
  }

  (*server_sock) = socket(AF_INET, TCP_SOCKETS, 0);
  //(*server_sock) = socket(af_inet, tcp_sockets, so_reuseaddr);
  int optval = 1;
  setsockopt((*server_sock), SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
  
  /* set the parameters for the server */
  server_addr->sin_family = AF_INET;
  server_addr->sin_port = htons(tcp_port);
  server_addr->sin_addr.s_addr = INADDR_ANY;

  /* bind the server to the client */
  int ret_val = bind(*server_sock, (struct sockaddr *)server_addr, server_addr_len);
  if(ret_val < 0) {
    perror("error:bind()\n");
    close(*server_sock); 
    exit(1);
  }

  /* start listening for the connection on the socket 
   * currently, limit the backlog connections to 5. this value will be 
   * maxed out depending on the value that is in /proc/sys/ipv4/tcp_max_syn_backlog
  */
  ret_val = listen(*server_sock, 5);
  if(ret_val < 0) {
    perror("error:listen()\n");
    exit(1);
  }
  return;
}

blacklist_status hostname_to_ip(char * hostname , char* ip)
{
  struct hostent *he;
  struct in_addr **addr_list;
  static uint16_t ip_cache_count = 0;

  /* Check if the hostname and ip is already cached or not */
  for(int i = 0; i<ip_cache_count; i++) {
#ifdef IP_CACHE_DEBUG
    printf("cache_he.h_name: %s\n", cache_he[i].h_name);
    printf("cache_he.h_addrtype: %d\n", cache_he[i].h_addrtype);
    printf("hostname passed: %s\n", hostname);
#endif
   
    if(!strcmp(hostname, cache_he[i].h_name)) {
      addr_list = (struct in_addr **) cache_he[i].h_addr_list;
      for(int i = 0; addr_list[i] != NULL; i++) 
      {
        strcpy(ip , inet_ntoa(*addr_list[i]) );
      }
      return ACCEPT;
    }
    
    /* Iterate the blacklist to check if the ip/hostname is valid or not */
    for(int k = 0; k < blacklist_count; k++) {
      if(!strcmp(blacklist_array[k], cache_he[i].h_name)||\
          !(strcmp(blacklist_array[k], ip))) {
        printf("Reject Request!!\n");
        return REJECT;
      }
    }
  }

  if ((he = gethostbyname(hostname)) == NULL) 
  {
    // get the host info
    herror("gethostbyname");
    return REJECT;
  }

  /* cache the struct */
  memcpy(&cache_he[ip_cache_count++], he, sizeof(struct hostent));
  
  addr_list = (struct in_addr **) he->h_addr_list;
  
  for(int i = 0; addr_list[i] != NULL; i++) 
  {
    strcpy(ip , inet_ntoa(*addr_list[i]) );
  }
  
  /* Iterate the blacklist to check if the ip/hostname is valid or not */
  for(int k = 0; k < blacklist_count; k++) {
    if(!strcmp(blacklist_array[k], he->h_name)||\
        !(strcmp(blacklist_array[k], ip))) {
      printf("Reject Request!!\n");
      return REJECT;
    }
  }

  return ACCEPT;
}

void create_md5_hash(char *digest_buffer, char *buffer)
{
  unsigned char digest[MD5_DIGEST_LENGTH]; 
  memset(digest, '\0', sizeof(digest));

  MD5_CTX md5_struct;
  MD5_Init(&md5_struct);
  MD5_Update(&md5_struct, buffer, strlen(buffer));
  MD5_Final(digest, &md5_struct);
  
  //char md5string[33];
  //memset(md5string, '\0', sizeof(md5string));

  /* Store the MD5 hash as a string */
  for(int i = 0; i<MD5_DIGEST_LENGTH; i++) {
    sprintf(&digest_buffer[i*2], "%02x", digest[i]); 
  }
  //printf("md5string: %s\n", digest_buffer);

  return;
}

int check_dir_presence(const char *dir_name)
{
  struct stat st = {0};
  
  if(stat(dir_name, &st) == -1) {
    printf("%s directory does not exist, create now!\n", dir_name);
    int ret_val = mkdir(dir_name, 0777);
    if(ret_val < 0) {
      perror("");
      return 1;
    }
  }

  return 1;
}

void url_cache_check(parsed_url *url_request)
{
  cache_info file_cache;
  memset(&file_cache, '\0', sizeof(file_cache));

  /* Check for the existance of the cache folder and create it if required */
  if(getcwd(file_cache.pwd_string, sizeof(file_cache.pwd_string)) != NULL) {
    //printf("file_cache.pwd_string: %s\n", file_cache.pwd_string);
    check_dir_presence(CACHE_DIR);
  }

  /* Check if the file is present in the folder or not 
   * and if yes, get the time that the file was create 
   */
  char file_path[50];
  memset(file_path, '\0', sizeof(file_path));

  /* Store the file name as its hash value */
  sprintf(file_path, "./cache/%s", url_request->domain_name_hash);  
  printf("file_path: %s\n", file_path);

  char c_time[60];
  memset(c_time, '\0', sizeof(c_time));

  struct stat attribute;
  int stat_ret = stat(file_path, &attribute);
  if(stat_ret < 0) {
    printf("File does not exist!\n");
    url_request->cache_file = false;
    return;
  }

  time_t file_creation_time = attribute.st_mtime;
  
  time_t current_time;
  time(&current_time);
  
  time_t time_diff = current_time - file_creation_time;
  
  struct tm *time_info = NULL;
  time_info = localtime(&time_diff);
  
  uint32_t difference = (time_info->tm_min * 60) + (time_info->tm_sec);

  //printf("tm_min: %d\n", time_info->tm_min);
  //printf("tm_sec: %d\n", time_info->tm_sec);

  printf("proxy_timeout_value: %d time difference: %d\n", proxy_timeout_value,\
                                      difference);

  if(difference > proxy_timeout_value) {
    printf("Proxy timeout exceeded!\n");
    url_request->cache_file = false;
  } else {
    url_request->cache_file = true;
  }

  //sprintf(c_time, "%s\n", ctime(&trial));
  //printf("c_time: %s\n", c_time);

  return;
}

void send_file_from_webserver(int client_sock_num, parsed_url *url_request)
{
  /* Open a connection to the webserver */
  struct hostent *host_connection;
  struct sockaddr_in host_address;
  
  host_connection = gethostbyname(url_request->domain_name);
  if(!host_connection) {
    perror("HostConnection");
    return;
  }

  host_address.sin_family = AF_INET;
  host_address.sin_port = htons(80);
  memcpy(&host_address.sin_addr, host_connection->h_addr, host_connection->h_length);
  
  socklen_t addr_len = sizeof(host_address);

  int server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if(server_sock < 0) {
    perror("SOCKET");
    return;
  }

  int optval = 1;
  /* Set socket properties */
  setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, 4);

  int connect_ret = connect(server_sock, (struct sockaddr *)&host_address,\
                                  addr_len);
  if(connect_ret < 0) {
    perror("CONNECT: ");
    return;
  }

  /* Construct the GET request to send to the webserver */
  char server_request[300];
  memset(server_request, '\0', sizeof(server_request));

  if(url_request->filename[0] == '\0') {
    sprintf(server_request, "GET / %s\r\nHost: %s\r\nAccept: */*\r\nConnection: keep-alive\r\n\r\n",\
        url_request->http_version, url_request->domain_name);
  } else {
    sprintf(server_request, "GET /%s %s\r\nHost: %s\r\nAccept: */*\r\nConnection: keep-alive\r\n\r\n",\
        url_request->filename, url_request->http_version, url_request->domain_name);
  }
  
  printf("server_request: %s\n", server_request);

  int send_bytes = send(server_sock, server_request, sizeof(server_request), 0);
  if(send_bytes < 0) {
    perror("SEND:");
    return;
  }

  /* Get the response from the webserver and print it on the console */
  int32_t recv_bytes = 0;
  char server_recv_buffer[MAX_BUFFER_SIZE];
  memset(server_recv_buffer, '\0', sizeof(server_recv_buffer));

  /* Check to see if the file needs to be cached */
  FILE *cache_fp = NULL;
  
  if(url_request->cache_file == false) {
    char file_path[100];
    memset(file_path, '\0', sizeof(file_path));
    
    sprintf(file_path, "./%s/%s", CACHE_DIR, url_request->domain_name_hash);  
    printf("opening file_path: %s\n", file_path);
    cache_fp = fopen(file_path, "wb");
    if(cache_fp == NULL) {
      perror("FILE_ERROR: ");
      return;
    }
  }

  do {
    memset(server_recv_buffer, '\0', sizeof(server_recv_buffer));
    recv_bytes = recv(server_sock, server_recv_buffer, sizeof(server_recv_buffer), 0);
   
    /* Send the data over to the client via the client socket */
    send(client_sock_num, server_recv_buffer, recv_bytes, 0);  

    /* Write the buffer to the cache file */
    fwrite(server_recv_buffer, 1, recv_bytes, cache_fp);
    
    //printf("%s\n", server_recv_buffer);
  } while(recv_bytes);

  url_request->cache_file = true;

  fclose(cache_fp);

  return;
}

void send_file_from_cache(int send_sock_num, parsed_url *url_request, char *temp_buffer)
{
  /* Create the path to the file */
  memset(url_request->domain_name_hash, '\0', sizeof(url_request->domain_name_hash));
  create_md5_hash(url_request->domain_name_hash, temp_buffer);
  
  char file_path[60];
  memset(file_path, '\0', sizeof(file_path));

  sprintf(file_path, "./%s/%s", CACHE_DIR, url_request->domain_name_hash);  
  printf("<%s>: file_path: %s\n", __func__, file_path);
  
  /* Open the file for reading */
  FILE *fp = NULL;
  fp = fopen(file_path, "rb");
  
  int32_t file_bytes_read = 0;

  char buffer[MAX_BUFFER_SIZE];
  memset(buffer, '\0', sizeof(buffer));
 
  /* Send the cached chunks to the client */
  do {
    file_bytes_read = fread(buffer, 1, MAX_BUFFER_SIZE, fp);
    send(send_sock_num, buffer, MAX_BUFFER_SIZE, 0);
  } while(file_bytes_read);

  /* release the file pointer once you're done sending */
  fclose(fp);

  return;
}

void *parse_client_request(void *accept_socket_number)
{
  int *accept_socket = (int *)accept_socket_number;

  char client_response[2048];
  memset(client_response, '\0', sizeof(client_response));
  
  char telnet_string[50];
  memset(telnet_string, '\0', sizeof(telnet_string));

  int recv_ret = recv((*accept_socket), client_response, sizeof(client_response), 0);
  if(recv_ret < 0) {
    perror("RECV\n");
    return NULL;
  }

  parsed_url url_request;
  memset(&url_request, '\0', sizeof(url_request));
  printf("socket_num: %d\n", *accept_socket);

  char temp_buffer[800];
  memset(temp_buffer, '\0', sizeof(temp_buffer));

  printf("client_response: %s\n", client_response);

  /* Parse the incoming client request */
  sscanf(client_response, "%s %s %s", url_request.client_command,\
      temp_buffer, url_request.http_version);
  /*printf("method: %s\nurl: %s\nhttp_version: %s\n", url_request.client_command,\
                    url_request.domain_name, url_request.http_version);*/
  
  if(strcmp(url_request.client_command, "GET")) {
    /* The request type is not supported for this implementation */
    int send_ret = send(*accept_socket, respond_400_error,\
        strlen(respond_400_error), 0);
    if(send_ret < 0) {
      perror("SEND");
      return NULL;
    }
    /* Return */
    return NULL;
  }

  /* Cleanup the URL and remove the slashes from it */
  char *temp = strstr(temp_buffer, "//");  
  temp = temp + 2;
  for(int i = 0; i<strlen(temp); i++) {
    if(temp[i] == '/')
      break;
    url_request.domain_name[i] = temp[i];
  }
 
  //char temp_out_array[50];
  //memset(temp_out_array, '\0', sizeof(temp_out_array));
  //memcpy(temp_out_array, (temp+2), (strlen(temp+2)-1));
  //memset(url_request.domain_name, '\0', sizeof(url_request.domain_name));
  //memcpy(url_request.domain_name, temp_out_array, strlen(temp_out_array));

  /* Check if there is a filename/path extension to the URL */
#if 1
  int j = 0;
  while(j != strlen(temp)) {
    if(temp[j] == '/') {
      strcpy(url_request.filename, &temp[j+1]);
      /* Change the URL as well */
      //url_request.domain_name[j] = '\0';
      break;
    }
    j++;
  }
#endif

  /* Get the IP address for the hostname */
  blacklist_status status = hostname_to_ip(url_request.domain_name, url_request.ip_addr);
  if(status == REJECT) {
    printf("Nope... Not sending anything, EXIT!!\n");
    send(*accept_socket, respond_403_error, strlen(respond_403_error), 0);
    close(*accept_socket);
    pthread_exit(NULL);
  }

  /* Get the MD5 hash for the passed URL */
  memset(url_request.domain_name_hash, '\0', sizeof(url_request.domain_name_hash));
  create_md5_hash(url_request.domain_name_hash, temp_buffer);
  
  /* Check to see if the URL is still cached or not */
  url_cache_check(&url_request);

#ifdef DEBUG_URL_PARSER
  printf("<%lu>: url_request.domain_name: %s\n", strlen(url_request.domain_name),\
                                              url_request.domain_name);
  printf("url_request.filename: %s\n", url_request.filename);
  printf("<%lu>:url_request.domain_name_hash: %s\n", strlen(url_request.domain_name_hash),\
                    url_request.domain_name_hash);
  printf("url_request.ip_addr: %s\n", url_request.ip_addr);
  printf("url_request.domain_name: %s\n", url_request.domain_name);
  printf("url_request.client_command: %s\n", url_request.client_command);
  printf("url_request.http_version: %s\n", url_request.http_version);
  printf("url_request.cache_file: %d\n", url_request.cache_file);
#endif

 
  /* If the cache is valid, don't start the webserver and send the file from the cache */
  if(url_request.cache_file == true) { 
    printf("++++++++++++++++++FROM CACHE+++++++++++++++++++\n");
    send_file_from_cache(*accept_socket, &url_request, temp_buffer);
  } else {
    send_file_from_webserver(*accept_socket, &url_request);
  }
 
  close(*accept_socket);
  pthread_exit(NULL);

  return NULL;
}

int get_client_request(int *accept_socket, int server_socket,\
                        struct sockaddr_in *addr, socklen_t addrlen)
{
  (*accept_socket) = accept(server_socket, (struct sockaddr *)addr, &addrlen);
  if(accept_socket < 0) {
    perror("ERROR:accept()\n");
    return -1; 
  }

  pthread_t thread_pid;
  pthread_create(&thread_pid, NULL, parse_client_request, accept_socket);
  pthread_join(thread_pid, NULL);

  return 0;
}


void read_blacklist_file(void)
{
  FILE *fp = NULL;
  fp = fopen("./ip_blacklist", "rb");
  if(fp == 0) {
    fprintf(stderr, "filed to open the blacklist file\n");
    exit(0);
  }

  int i = 0;
  while(i < 10 && fgets(blacklist_array[blacklist_count], sizeof(blacklist_array[0]), fp)) {
    blacklist_array[blacklist_count][strlen(blacklist_array[blacklist_count]) - 1] = '\0';
    blacklist_count++;
  }

  fclose(fp);

  return;
}


int main(int argc, char *argv[])
{
  /* Get the port number for the proxy server from the 
   * command line
   */
  if(argc > 3 || argc <= 1) {
    printf("Please enter valid command line arguments\n./web_proxy <port_num>\n");
    exit(0);
  }

  uint16_t proxy_portnum = atoi(argv[1]);
  
  if(argv[2] != NULL) {
    proxy_timeout_value = atoi(argv[2]);
  }

  /* Read blacklist file and store in array */
  read_blacklist_file();
  printf("%s\n", blacklist_array[3]);

  int server_sock = 0;
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));

  /* Initialize the socket values to -1 */
  memset(client_slots, -1, sizeof(client_slots));

  create_server_connections(&server_sock, &address, sizeof(address), proxy_portnum);

  socklen_t addrlen = sizeof(address);

  /* Get the client response and store it here */
  char client_response[2048];
  memset(client_response, '\0', sizeof(client_response));

  int16_t active_connections = 0;

  while(1)
  {
    /* Cleanup the response buffer */
    memset(client_response, '\0', sizeof(client_response));
  
    get_client_request(&client_slots[active_connections], server_sock,\
                        &address, addrlen);

    active_connections = (active_connections+1) % MAX_CONNECTIONS;
    printf("active_connections: %d\n", active_connections); 
  }

  close(server_sock);

  return 0;
}

