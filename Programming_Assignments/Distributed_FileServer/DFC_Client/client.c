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

  /* Parse the client config file and store the data in structures */
  client_config_data_t client_data;
  Parse_Client_Config_File(argv[1], &client_data);
  //Dump_Client_Data(&client_data, 4);
 

  
  return 0;
}



