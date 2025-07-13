#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "config.h"

typedef struct cline
{ int class_id;
  int index;
  conf config;
} cline;

static cline *config=NULL;
static int size;
 
static void initconf(void)
{ config = (cline *)calloc(4096, sizeof(cline));
  FILE *cfile = fopen(CONF_DIR "telaradb.conf", "r");
  if (!cfile)
  { printf("Failed to open %s\n", CONF_DIR "telaradb.conf");
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
      if (sscanf(key, "%d.%d", &c->class_id, &c->index) == 1)
	c->index = -1;
      char *name = strtok(NULL, " \t\n");
      c->config.name = strdup(name);

      char *opt = strtok(NULL, " \t\n");
      if (opt)
      { char o[16];
	o[0] = 0;
        char val[32];
        sscanf(opt, "%[^:]:%s", o, val);
        if (!strcmp("istring", o))
          c->config.istring = atoi(val); 
        else if (!strcmp("listring", o))
	  c->config.listring = atoi(val);
	else if (!strcmp("idstring", o))
	{ c->config.type = "idstring";
	  c->config.link = atoi(val);
	}
        else if (!strcmp("link", o))
        { c->config.link = atoi(val);
	  c->config.linkt = 0;
        }
	else if (!strcmp("alink1", o))
        { c->config.link = 1;
          c->config.linkt = 1;
	  c->config.type = strdup(val);
        }
	else if (!strcmp("alink2", o))
        { c->config.link = 1;
          c->config.linkt = 2;
	  c->config.type = strdup(val);
        }
        else
          c->config.type = strdup(o);
      }
      nr++;
    }
  }
  size = nr;
  fclose(cfile);
}

conf *getconf(int id, int index)
{ if (!config)
    initconf();
  for (int i=0; i<size; i++)
    if (config[i].class_id == id && config[i].index == index)
      return &config[i].config;

  return NULL;
}

int getindex(int id, char *name)
{ if (!config)
    initconf();

  for (int i=0; i<size; i++)
   if (config[i].class_id == id && !strcmp(config[i].config.name, name))
     return config[i].index;

  return -1;
}
