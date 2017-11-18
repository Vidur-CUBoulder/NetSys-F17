#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<stdbool.h>


int main()
{
  char chunk_storage[100000];
  memset(chunk_storage, '\0', sizeof(chunk_storage));
  
  FILE *fp = NULL;
  fp = fopen("./w.png", "rb");
  if(fp == NULL) {
    printf("Unable to open file!\n");
    perror("");
    return 0;
  }
  
  FILE *fp_write = NULL;
  fp_write = fopen("./xor_temp.txt", "wb");
  
  uint32_t read_count = 0;
  uint32_t file_size = 0;
  fseek(fp, 0L, SEEK_END);
  file_size = ftell(fp);
  rewind(fp);

  read_count = fread(chunk_storage, 1, file_size, fp);
  printf("read_count: %d\n", read_count);

  /* XOR the array */
  char xor_array[file_size];
  memset(xor_array, '1', sizeof(xor_array));
  for(int i = 0; i<sizeof(xor_array); i++) {
    xor_array[i] = xor_array[i] ^ chunk_storage[i];
    //printf("%c ", xor_array[i]);
  }

  /* Write to a temp file */
  fwrite(xor_array, 1, file_size, fp_write);

  fclose(fp_write);
  fclose(fp);

  return 0;
}

