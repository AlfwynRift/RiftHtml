#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "common.h"
#include "database.h"
#include "discname.h"
#include "dconf.h"
#include "html.h"
#include "tpath.h"

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

static void output(char *tag, char *val, int nesting)
{ printf("<tr>");
  for (int i=0; i<4; i++)
  { printf("<td>");
    if (i == nesting)
      printf("x");
    printf("</td>");
  }
  printf("<td>%s</td>", tag);
  if (!strcmp(val, "\n"))
    printf("<td></td>");
  else
    printf("<td>%s</td>", val);
  printf("</tr>\n");
}

static void print(FILE *xml, char *class, int nesting)
{ char line[2048], tag[64], val[2048], val2[2048];
  while (1)
  { fgets(line, 2047, xml);
    val[0]=0;
    sscanf(line, " <%[^>]>%[^<]", tag, val);
    while (strlen(val)>1 && val[strlen(val)-1]=='\n')
    { fgets(line, 2048, xml);
      val2[0]=0;
      sscanf(line, "%[^<]", val2);
      if (!val2[0])
	break;
      strcat(val, val2);
    } 
    if (tag[0] == '/')
      return;
    if (nesting && !strcmp(tag, "ItemKey"))
    { uint64 i1=0, i2=0;
      sscanf(val, "%16lx%lx", &i1, &i2);
      char *val2 = strdup(val);
      char *name=discname("ItemKey", i2, i1);
      if (name)
        sprintf(val, "<a href=\"discovery.cgi?cat=ItemKey&id=%s\">%s</a>", val2, name);
      else
        sprintf(val, "<span style=\"color:#FF0000;\">%s</span>", val2);
    } else
    { dconf *c=getdconf(class, tag);
      if (c && c->cat)
      { int id = 0;
	if (!strcmp(c->cat, "icon"))
	{ int ind=0;
	  char *name = strdup(val);
	  for (int i=0; i<strlen(name); i++)
            if (name[i]=='\\')
              ind=i+1;
          int len=strlen(name+ind);
          tps_member *tm = tpath_obj("#6009:[.0='%s']/.1", name+ind);
          if (tm)
          { sprintf(val, "<img src=\"" TELARADB "images/%s.png\" style=\"vertical-align: middle;\"> <a href=\"telaradb.cgi?id=6009&key=%d\" style=\"vertical-align: middle;\">%s</a>", tm->m.data.s, tm->key, name);
          }
	  free(name);
	}
	else
	{ if (val)
	    id = atoi(val);
          char *name=NULL;
	  if (id)
	    name = discname(c->cat, id, 0);
          if (name && id)
	    sprintf(val, "<a href=\"discovery.cgi?cat=%s&id=%d\">%s</a>", c->cat, id, name);
          else
            sprintf(val, "<span style=\"color:#FF0000;\">%d</span>", id);
	}
      }
      else if (c && c->link)
      { char *name=getname(c->link, atoi(val));
        char *val2 = strdup(val);
        if (name)
          if (name[0])
            sprintf(val, "<a href=\"telaradb.cgi?id=%d&key=%s\">%s</a>", c->link, val2, name);
          else
            sprintf(val, "<a href=\"telaradb.cgi?id=%d&key=%s\">%s</a>", c->link, val2, val2);
        else
          sprintf(val, "<span style=\"color:#FF0000;\">%s</span>", val2);
      }
    }
    output(tag, val, nesting);
    if (val[0]=='\n' && tag[strlen(tag)-1]!='/')
      print(xml, tag, nesting+1);
  }
}

char *release;

int main(int argc, char **argv)
{ char *query;
  release = RELEASE;

  query = getenv("QUERY_STRING");

  html_header("Discoveries", "", "", "");

  static char *cats[] = { "Achievement", "ArtifactCollection", "Item", "ItemKey", "NPC", "Quest", "Recipe", NULL };
  static char *mcats[] = { "achievement", "artifactset", "item", "item", "npc", "quest", "recipe", NULL };
  char *id=NULL;
  char *cat=NULL;
  char *mcat=NULL;
  char *arg=strtok(query, "&");
  do
  { if (!strncmp(arg, "cat=", 4))
    { cat=strdup(arg+4);
      int i;
      for (i=0; cats[i]; i++)
	if (!strcmp(cat, cats[i]))
	{ mcat = mcats[i];
	  break;
	}
      if (!cats[i])
      { printf("Unkown category\n");
	return 0;
      }
    }
    else if (!strncmp(arg, "id=", 3))
      id=arg+3;
  } while (arg=strtok(NULL, "&"));

  if (!cat)
  { printf("Missing category parameter cat\n");
    return 0;
  }
  char *scat = cat;
  if (!strcmp(cat, "ItemKey"))
    scat="Item";
  printf("file: %ss.xml id: %s\n", scat, id);

  char inname[128];
  snprintf(inname, 127, "%s%ss.index", DISCOVERY_DIR, cat);
  int infile = open(inname, O_RDONLY);
  FILE *inf = fdopen(infile, "r");
  fseek(inf, 0, SEEK_END);
  int insize = ftell(inf);
  byte *data = mmap(NULL, insize, PROT_READ, MAP_PRIVATE, infile, 0);

  uint32 size = *(uint32 *)data;
  uint32 *keys = (uint32 *)data+1;
  uint32 *s;

  if (!strcmp(cat, "ItemKey"))
  { uint64 i1=0, i2=0;
    keysize = 16;
    sscanf(id, "%16lx%lx", &i1, &i2);
    uint64 k[2];
    k[0] = i2;
    k[1] = i1;
    s = bsearch(&k, keys, size, 16, &cmp);
    cat = "Item";
  }
  else
  { keysize=4;
    int sid=atoi(id);
    s = bsearch(&sid, keys, size, 4, &cmp);
  }

  if (!s)
  { printf("No such id!\n");
    return 0;
  }
 
  int index = (s-keys)*4/keysize;
  uint32 *str_off = keys + (1+keysize/4)*size;
  uint32 *xml_off = keys + size*keysize/4;
  int off = xml_off[index];
  fclose(inf);
  munmap(data, insize);
  close(infile);

  int dbid=0;
  if (!strcmp(cat, "Achievement"))
    dbid=2200;
  else if (!strcmp(cat, "ArtifactCollection"))
    dbid=10701;

  if (dbid)
    printf("<a href=\"telaradb.cgi?id=%d&key=%d\">telara.db</a>\n", dbid, atoi(id));

  snprintf(inname, 127, "%s%ss.xml", DISCOVERY_DIR, cat);
  FILE *xml = fopen(inname, "r");
  fseek(xml, off, SEEK_SET);

  if (!strcmp(cat, "Item"))
  { char line[128];
    fgets(line, 127, xml);
    id = malloc(64);
    sscanf(line, " <ItemKey>%[^<]", id);
    fseek(xml, off, SEEK_SET);
  }

  if (!strcmp(cat, "Item") || !strcmp(cat, "ItemKey"))
    printf(" <a href=\"" TELARADB "#/items/%s\">telaradb.com</a>\n", id);

  printf(" <a href=\"" MAGELO "%s/%s\">magelo.com</a>\n", mcat, id);
 
  printf("<table>\n");
  printf("<tr><th colspan=4>Nesting</th><th>Name</th><th>Value</th></tr>\n");

  print(xml, cat, 0);
 
  printf("</table>\n");
  printf("</body></html>\n"); 

  return 0;
}
