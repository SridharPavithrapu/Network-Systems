#include <openssl/md5.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char**argv)
{
  FILE *fh;
  long filesize;
  unsigned char *buf;
  unsigned char *md5_result = NULL;
  int i;
  if (argc < 2)
  {
    printf("Usage: md5_test <filename>\n");
    return 1;
  }


  fh = fopen(argv[1], "r");
  fseek(fh, 0L, SEEK_END);
  filesize = ftell(fh);
  fseek(fh, 0L, SEEK_SET);
  buf = malloc(filesize);
  fread(buf, filesize, 1, fh);
  fclose(fh);
  md5_result = malloc(MD5_DIGEST_LENGTH);	
	printf("len:%d",MD5_DIGEST_LENGTH);
  MD5(buf, filesize, md5_result);
  printf("MD5 (%s) = ", argv[1]);
  for (i=0; i < MD5_DIGEST_LENGTH; i++)
  {
    printf("%02x",  md5_result[i]);

  }
	char c[2];
	sprintf(c,"%02x",md5_result[MD5_DIGEST_LENGTH-1]);
printf("\ncahr:%s",c);
	printf("\nc[0]:%c,c[1]:%c,c[2]:%c",c[0],c[1],c[2]);
  printf("\n");
	int choice = atoi(c[1]);
	printf("%d",choice);
  free(md5_result);
  free(buf);
  return 0;
}
