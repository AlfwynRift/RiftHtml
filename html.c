#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "common.h"

void html_header(char *title, char *id, char *key)
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
  printf(data, title, id, key);

  munmap(data, insize);
  fclose(inf);
  close(infile); 
}
