#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include "common.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef unsigned char byte;

typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

static int keysize;

static int cmp(const void *a, const void *b)
{ if (keysize == 4)
  { int32 na = *(int32 *)a;
    int32 nb = *(int32 *)b;
    if (na<nb)
      return -1;
    if (na>nb)
      return 1;
  }
  else
  { uint64 ha = *((uint64 *)a+1);
    uint64 hb = *((uint64 *)b+1);
    if (ha<hb)
      return -1;
    if (ha>hb)
      return 1;
    uint64 la = *(uint64 *)a;
    uint64 lb = *(uint64 *)b;
    if (la<lb)
      return -1;
    if (la>lb)
      return 1;
  }
  return 0;
}

char *discname(const char *Cat, long id, long id_hi)
{ byte *data;
  char *ret;

  char inname[128];
  snprintf(inname, 127, "%s%ss.index", DISCOVERY_DIR, Cat); 
  int infile = open(inname, O_RDONLY);
  FILE *inf = fdopen(infile, "r");
  fseek(inf, 0, SEEK_END);
  int insize = ftell(inf);
  data = mmap(NULL, insize, PROT_READ, MAP_PRIVATE, infile, 0); 

  uint32 size = *(uint32 *)data;
  uint32 *keys = (uint32 *)data+1;
  uint32 *s;

  if (!strcmp(Cat, "ItemKey"))
  { keysize = 16;
    uint64 k[2];
    k[0] = id;
    k[1] = id_hi;
    s = bsearch(&k, keys, size, 16, &cmp);
  }
  else
  { keysize=4;
    s = bsearch(&id, keys, size, 4, &cmp);
  }
  if (!s)
    ret = NULL;
  else
  { int index = (s-keys)*4/keysize;
    uint32 *str_off = keys + (1+keysize/4)*size;
    ret = strdup(data + str_off[index]);
  }
  munmap(data, insize);
  return ret;
}

#if 0
int main(int argc, char **argv)
{ uint64 i1=0, i2=0;
  sscanf(argv[1], "%16lx%lx", &i1, &i2);
printf("%016lx %016lx\n", i1, i2);
  char *name = discname("ItemKey", i2, i1);
  if (name)
    printf("%s\n", name);
  return 0;
}
#endif
