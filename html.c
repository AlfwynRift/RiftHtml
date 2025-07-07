#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "common.h"

void html_header(char *title, char *id, char *key, char *tpath)
{ int infile = open(CONF_DIR "html_header", O_RDONLY);
  if (infile == -1)
  { printf("Failed to open %s\n", CONF_DIR "html_header");
    exit(0);
  }
  FILE *inf = fdopen(infile, "r");
  fseek(inf, 0, SEEK_END);
  int insize = ftell(inf);
  char *data = mmap(NULL, insize, PROT_READ|PROT_WRITE, MAP_PRIVATE, infile, 0);
  data[insize] = 0;

  printf("Content-type: text/html\n\n");
  printf(data, title, id, key, tpath);

  munmap(data, insize);
  fclose(inf);
  close(infile); 
}

char *url_decode(char *param)
{ int out=0;
  for (int i=0; i<strlen(param); i++)
  if (param[i] == '%')
  { int num='?';
    sscanf(param+i, "%%%02X", &num);
    param[out++] = num;
    i += 2;
  }
  else
    param[out++] = param[i]; 
  param[out] = 0;

  return param;
}
