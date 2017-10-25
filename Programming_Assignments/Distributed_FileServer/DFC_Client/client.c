#include"common.h"

int main(int argc, char *argv[])
{
  /* Get the input from the command line */
  char dfc_config_file[8];
  memset(dfc_config_file, '\0', sizeof(dfc_config_file));
  
  if(argc == 1) {
    printf("Configuration file not passed!\n");
    exit(0);
  }

  size_t read_line = 0;
  uint8_t cntr_words = 0;
  char *line = (char *)malloc(50);

  FILE *fp_config_file = NULL;
  fp_config_file = fopen(argv[1], "rb");
  if(fp_config_file == NULL) {
    fprintf(stderr, "%s: ", argv[1]);
    perror("");
    exit(0);
  }

  printf("argc: %d; argv[1]: %s\n", argc, argv[1]);

  while( (read_line = getline(&line, &read_line, fp_config_file)) != -1) {
    //printf("%s", line);
    /* get the server name, IP and Port number from the extracted line */ 
    cntr_words = 0;
    char *buffer = strtok(line, " \n");
    
    if(!strcmp(buffer, "Username:")) {
      printf("Username then!\n");
      continue;
    }

    if(!strcmp(buffer, "Password:")) {
      printf("Password then!\n");
      continue;
    }
    
    while(buffer != NULL) {
      switch(cntr_words)
      {
        case 0:
          /* This is just the word "Server"; do nothing */
          printf("0. %s\n", buffer);
          break;

        case 1:
          /* This is the DFS server name.
           * Store the names of the servers in memory 
           */
          printf("1. %s\n", buffer);
          break;

        case 2:
          /* This is the IP address and the port number */
          printf("2. %s\n", buffer);
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

  return 0;
}



