#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "database.h"
#include "lstring.h"
#include "huffman.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct key
{ int key;
  unsigned int off;
} key;

static int cmp(const void *kk, const void *ee)
{ int k = ((key *)kk)->key;
  int c = ((key *)ee)->key;

  if (k<c)
    return -1;
  else if (k>c)
    return 1;
  else
    return 0;
}

char *nlstring(int id)
{ static key *keys = NULL;
  static node *tree;
  static int nr;
  static byte *data;

  if (!keys)
  { char filename[128];
    snprintf(filename, 127, "%s%s/lang_english.cds", DATA_DIR,  release);
    int infile = open(filename, O_RDONLY);
    if (infile == -1)
    { printf("Failed to open %s\n", filename);
      exit(0);
    }
    FILE *inf = fdopen(infile, "r");
    fseek(inf, 0, SEEK_END);
    int insize = ftell(inf);
    data =  mmap(NULL, insize, PROT_READ, MAP_PRIVATE, infile, 0); 
    fclose(inf);
    close(infile);

    nr = *(uint32 *)data;
    data += 4;
    keys = calloc(nr, sizeof(key));

    tree = buildTree((uint32 *)data);
    data += 1024;

    for (int i=0; i<nr; i++)
    { keys[i].key = *(int32 *)data;
      data += 4;
      keys[i].off = leb128(&data);
    }
  }

  char *ret=NULL;
  key search;
  search.key = id;

  key *k = (key *)bsearch(&search, keys, nr, sizeof(key), cmp);

  if (k)
  { byte *d = data + k->off;
    int s1 = leb128(&d);
    int s2 = leb128(&d);
    byte *o = decompress(d, s2, NULL, tree);
    int v = leb128(&o);      // classcode
    v = leb128(&o);          // code 11, map
    v = leb128(&o);          // types + length
    v = leb128(&o);          // value
    v = leb128(&o);          // classcode
    v = leb128(&o);          // code 6, string
    v = leb128(&o);          // len

    ret = o;
    ret[v] = 0;
  }

  return ret;
}

char *lstring(obj *o7703)
{ char *ret;

  ret = nlstring(o7703->members->data.si);
  if (!ret && o7703->members[1].type == T_STRING)
  { char *s=o7703->members[1].data.s;
    ret = calloc(strlen(s)+8, 1);
    sprintf(ret, "<i>%s</i>", s);
  }

  return ret;
}
