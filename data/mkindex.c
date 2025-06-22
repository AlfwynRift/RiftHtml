#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef unsigned char byte;

typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef struct entry
{ uint64 key;
  uint64 key_hi;
  uint32 off;
  uint32 str_off;
  char *str;
} entry;

static int keysize = 4;

static int cmp(const void *a, const void *b)
{ entry *ea = (entry *)a;
  entry *eb = (entry *)b;

  if (keysize==4)
  { int32 na = ea->key;
    int32 nb = eb->key;
 
    if (na<nb)
      return -1;
    if (na>nb)
      return 1;
  }
  else
  { if (ea->key_hi < eb->key_hi)
      return -1;
    if (ea->key_hi > eb->key_hi)
      return 1;
    if (ea->key < eb->key)
      return -1;
    if (ea->key > eb->key)
      return 1;
  }
  return 0;
}

int main(int argc, char **argv)
{ char inname[32], outname[32];
  char *idtag, *cat=argv[1];

  if (!cat)
  { printf("Usage: %s Achievement|ArtifactCollection|Item|ItemKey|NPC|Recipe|Quest\n", argv[0]);
    return 0;
  }

  if (!strcmp(cat, "Item"))
    idtag=" <TemplateId>%d";
  else if (!strcmp(cat, "ItemKey"))
  { idtag=" <ItemKey>%x";
    keysize=16;
  }
  else if (!strcmp(cat, "Quest"))
    idtag=" <QuestId>%d";
  else if (!strcmp(cat, "NPC") || !strcmp(cat, "Achievement") || !strcmp(cat, "ArtifactCollection") || !strcmp(cat, "Recipe"))
    idtag=" <Id>%d";
  else
  { printf("Unknown categrory \"%s\"\n", cat);
    return 0;
  }

  sprintf(outname, "%ss.index", cat);
  if (!strcmp(cat, "ItemKey"))
    cat="Item";
  sprintf(inname, "%ss.xml", cat);

  FILE *infile = fopen(inname, "r");
  if (!infile)
  { printf("Cannot open input file \"%s\"\n", inname);
    return 0;
  }
  entry *data = calloc(sizeof(entry), 256000);
  data++;
  char line[1024], name[256];
  int id, i=-1; 
  
  char tagline[32];
  sprintf(tagline, "  <%s>\n", cat);
  while (fgets(line, 256, infile))
  { if (!strcmp(line, tagline))
      data[++i].off = ftell(infile);
    else if (!data[i].str && (!strcmp(line, "    <Name>\n") || !strcmp(line, "    <PrimaryName>\n")))
    { fgets(line, 256, infile);
      sscanf(line, " <English>%[^<]", name);
      data[i].str = strdup(name);
    }
    else if (!data[i].key && sscanf(line, idtag, &id)==1)
    { if (keysize==4)
        data[i].key = id;
      else
      { uint64 i1=0, i2=0;
	sscanf(line, " <ItemKey>%16lx%lx", &i1, &i2);
	data[i].key = i2;
	data[i].key_hi = i1;
      }    
    }
  }
  fclose(infile);

  uint32 size = i;
  qsort(data, size , sizeof(entry), &cmp);
  
  FILE *outfile = fopen(outname, "w");
  fwrite(&size, 4, 1,  outfile);
  int stroff = 4 + (keysize+8)*size;
  for (int i=0; i<size; i++)
  { if (keysize==4)
      fwrite(&data[i].key, 4, 1, outfile);
    else
    { fwrite(&data[i].key, 8, 1, outfile);
      fwrite(&data[i].key_hi, 8, 1, outfile);
    }
    data[i].str_off = stroff;
    stroff += 1+strlen(data[i].str);
  }
  for (int i=0; i<size; i++)
    fwrite(&data[i].off, 4, 1, outfile);
  for (int i=0; i<size; i++)
    fwrite(&data[i].str_off, 4, 1, outfile);
  for (int i=0; i<size; i++)
  { fputs(data[i].str, outfile);
    putc('\0', outfile);
  }
  fclose(outfile);

  return 0;
}
