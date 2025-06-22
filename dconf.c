#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "dconf.h"

typedef struct cline
{ char *class;
  char *tag;
  dconf config;
} cline;

static cline *config=NULL;
static int size;

static void initconf(void)
{ config = (cline *)calloc(256, sizeof(cline));
  FILE *cfile = fopen(CONF_DIR "discovery.conf", "r");
  if (!cfile)
  { printf("Failed top open %s\n", CONF_DIR "discovery.conf");
    exit(0);
  }
  char line[80];
  int nr=0;
  for (int i=0; fgets(line, 79, cfile); i++)
  { for (int j=0; line[j]; j++)
      if (line[j] == '#')
      { line [j] = 0;
        break;
      }
    char *key;
    if (key = strtok(line, " \t\n"))
    { cline *c = config+nr;
      char class[64], tag[64];
      class[0]=0;
      tag[0] = 0;
      sscanf(key, "%[^.].%s", class, tag);
      if (tag[0])
      { c->class = strdup(class); 
	c->tag = strdup(tag);
      }
      else
	c->tag = strdup(class);

      char *opt = strtok(NULL, " \t\n");
      if (opt)
      { char o[16], cat[64];
        sscanf(opt, "%[^:]:%s", o, cat);
        if (!strcmp("cat", o))
          c->config.cat = strdup(cat);
        else if (!strcmp("link", o))
          c->config.link = atoi(cat);
      }
      nr++;
    }
  }
  size = nr;
}

dconf *getdconf(char *class, char *tag)
{ if (!config)
    initconf();
  for (int i=0; i<size; i++)
    if (!strcmp(tag, config[i].tag) && (!config[i].class || !strcmp(config[i].class, class)))
      return &config[i].config;

  return NULL;
}
