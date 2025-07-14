#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "common.h"
#include "database.h"
#include "tpath.h"

static char buf[1024];
static jmp_buf env;
static int ret_size;
static tset *ret_set;
int tpath_errno;
char *tpath_errmsg;

static void tpath_error(int n, char *msg, ...)
{ va_list args;
  va_start(args, msg);
  vsnprintf(buf, 1023, msg, args);
  va_end(args);

  tpath_errno = n;
  tpath_errmsg = buf;
}

static void tpath_err(int n, char *msg, ...)
{ va_list args;
  va_start(args, msg);
  vsnprintf(buf, 1023, msg, args);
  va_end(args);

  tpath_errno = n;
  tpath_errmsg = buf;
  longjmp(env, 1);
}

static int id;
static int key;
static char *p;

static void add_to_set(member m)
{ //if (m.type == T_NONE)
  //  return;

  if (ret_set->nmemb == ret_size)
  { ret_size *= 2;
    ret_set = realloc(ret_set, sizeof(tset)+ret_size*sizeof(obj));
  }
  tps_member tps_m;
  tps_m.m = m;
  if (id)
    tps_m.key = key;
  ret_set->members[ret_set->nmemb++] = tps_m;
}

static void tpath(obj *o, char *path);
static void tpath_match(obj *o, char *path);

static void tpath_matched(member m, char *path)
{ int t = m.type;
  if (1 || t != T_NONE)
  { if (*path)
    { if (t!=T_CLASS && t!=T_ARRAY && t!=T_MAP)
        tpath_err(102, "expected aggregate type, read code %d instead", m.code);
      if (*path == '[')
      { static tset *ret_set_bak=NULL;
        path++;
	int ret_size_bak = ret_size;
	if (ret_set_bak)
	  tpath_err(105, "nested conditions not supported");
	ret_set_bak = ret_set;
	ret_size = 1;
	ret_set = calloc(1, sizeof(tset)+sizeof(obj));
	int i;
	char *cpath = NULL;
	for (i=0; path[i]; i++)
	  if (path[i] == '=')
	  { cpath = strndup(path, i);
	    break;
	  }
	path += i+1;
	if (!cpath)
	  tpath_err(106, "expected '=' in condtion");
	tpath_match(m.data.o, cpath);
	free(cpath);
	if (ret_set->nmemb > 1)
	  tpath_err(107, "expression path matched multiple things");
	int mval;
	char *mstring = NULL;
        if (ret_set->nmemb == 1)
	{ member m = ret_set->members[0].m;
	  if (m.type==T_NUM || m.type==T_NONE)	// FIXME: default 0 assumed
	    mval = m.data.si;
	  else if (m.type == T_STRING)
	    mstring = m.data.s;
	  else
	    tpath_err(108, "only integer and string comparison supported");
	}
	int found = (ret_set->nmemb == 1);
	ret_size = ret_size_bak;
	ret_set = ret_set_bak;
	ret_set_bak = NULL;

	int matched = 0;
	if (mstring)
	{ char start = *path;
	  if (start!='"' && start!='\'')
	    tpath_err(109, "string literal starting with ' or \" expected, read '%c'", start);
	  char *lit = ++path;
	  while(*path && *path!=start)
	    path++;
	  if (!*path)
	    tpath_err(110, "closing %c not found", start);
	  path++;
	  matched = !strncmp(mstring, lit, path-lit-1);
	}
	else
	{ int val =  atoi(path);
	  while (isdigit(*path)) path++;
	  matched = (mval==val);
	}
	if (*path != ']')
	tpath_err(104, "expected closing ']', read '%c' instead", *path?*path:'0');
	path++;
	obj *o = m.data.o;
	if (found && matched)
        { if (*path)
	    tpath(o, path);
	  else
	    add_to_set(m);
	}
      }
      else
        tpath(m.data.o, path);
    }
    else
      add_to_set(m);
  }
}

static void tpath_match(obj *o, char *path)
{ if (*path == '.')
  { int index = atoi(++path);
    while (isdigit(*path)) path++;
    if (index<o->nmemb)
      tpath_matched(o->members[index], path);
  }
  else if (*path == '{')
  { int index = atoi(++path);
    while (isdigit(*path)) path++;
    if (*path != '}')
      tpath_err(101, "expexted closing '}', read '%c' instead", *path?*path:'0');
    path++;
    for (int i=0; i<=o->nmemb; i+=2)
      if (o->members[i].data.si == index)
	tpath_matched(o->members[i+1], path);
  }
  else if (*path == '@')
  { int id = atoi(++path);
    while (isdigit(*path)) path++;
    for (int i=0; i<=o->nmemb; i++)
      if (o->members[i].type==T_CLASS && o->members[i].data.o->class_id==id)
	tpath_matched(o->members[i], path);
  }
}

static void tpath_search(obj *o, char *path)
{ tpath_match(o, path);
  for (int i=0; i<=o->nmemb; i++)
  { int t = o->members[i].type;
    if (t==T_CLASS || t==T_ARRAY || t==T_MAP)
      tpath_search(o->members[i].data.o, path);
  }
}

static void tpath_cb(int k)
{ key = k;
  obj *o = read_obj(id, key);
  tpath(o, p);
}

static void tpath(obj *o, char *path)
{ //printf("Object: %p\tPath: %s \n", o, path);
  if (*path == '/')
  { if (path[1] == '/')
      tpath_search(o, path+2);
    else
      tpath_match(o, path+1);
  }
  else if (*path==0 || *path=='[')
  { member m;
    m.code = 9;
    m.type = T_CLASS;
    m.data.o = o;
    tpath_matched(m, path);
  }
  else
    tpath_search(o, path);
}

static tset *tpath_wrapper(char *path)
{ ret_set = NULL;
  tpath_errno = 0;
  tpath_errmsg = NULL;

  int pl = strlen(path);
  char *resource = strtok(path, ":");
  if (strlen(resource) == pl)
  { tpath_error(5, "missing seperator ':'");
    return NULL;
  }
  p = strtok(NULL, ":");
  if (!p)
    p="";
  obj *o = NULL;
  id = 0;
  switch (resource[0])
  { case '#':
    { id = atoi(strtok(resource+1, "/"));
      char *k = strtok(NULL, "/");
      if (k)
      { key = atoi(k);
        o = read_obj(id, key);
        if (!o)
        { tpath_error(4, "id/key pair not found in database");
	  return NULL;
        }
      }
      break;
    }

    case '@':
      tpath_error(3, "Resource method not iplemented");
      return NULL;

    case '/':
      { char filename[128];
        snprintf(filename, 127, "%s%s/%s", DATA_DIR,  release, resource+1);
        int infile = open(filename, O_RDONLY);
	if (infile == -1)
    	{ tpath_error(5, "Failed to open %s\n", filename);
      	  return NULL;
    	}
	FILE *inf = fdopen(infile, "r");
	fseek(inf, 0, SEEK_END);
	int insize = ftell(inf);
	byte *data =  mmap(NULL, insize, PROT_READ, MAP_PRIVATE, infile, 0); 
	fclose(inf);
	close(infile);

	o = decode_obj(data, insize);
      }
      break;

   default:
      tpath_error(2, "Undefined resource method");
      return NULL;
  }

  if (setjmp(env))
  { if (ret_set)
    { free(ret_set);
      ret_set = NULL;
      return NULL; 
    }
  }

  ret_set = calloc(1, sizeof(tset)+8*sizeof(obj));
  ret_size = 8;
  if (o)
    tpath(o, p);
  else
    read_all(id, &tpath_cb);
  return ret_set;
}

tset *tpath_set(const char *path, ...)
{ va_list args;
  va_start(args, path);
  vsnprintf(buf, 1023, path, args);
  va_end(args);

  return tpath_wrapper(buf);
}

tps_member *tpath_obj(const char *path, ...)
{ va_list args;
  va_start(args, path);
  vsnprintf(buf, 1023, path, args);
  va_end(args);

  tset *res = tpath_wrapper(buf);
  tps_member *ret;
  if (res)
  { switch (res->nmemb)
    { case 0:
        ret = NULL;
        break;

      case 1:
        ret = malloc(sizeof(tps_member));
        memcpy(ret, res->members, sizeof(tps_member));
        break;
       
      default:
        tpath_error(1, "Path matches more than one object");
	ret = NULL;
        break;
    }
    free(res);
  }
  else
    ret = NULL;

  return ret;
}

obj *tpath_tm2obj(tps_member *tm)
{ obj *o = calloc(1, sizeof(obj)+sizeof(member));
   o->members[0] = *(member *)tm;
   o->nmemb = 1;
   return o;
}
