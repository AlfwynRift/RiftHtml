#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include "common.h"
#include "database.h"
#include "config.h"
#include "lstring.h"
#include "discname.h"
#include "html.h"
#include "tpath.h"

static char *codenames[] = { "false", "true", "ulong", "slong", "4bytes", "8bytes", "string", "classend", "?", "class", "array", "map", "?", "?", "?", "?", "lstring", "istring", "listring", "link", "float", "int", "item", "quest", "NPC", "recipe", "wintime" , "color", "double", "nlstring" };

static void output(obj *o, int index, int indent, char *val, int pindex)
{ static int subclass;
  conf *c=NULL;
  if (pindex >= 0)
    c = getconf(o->class_id, pindex);
  else if (pindex < 0)
    c = getconf(o->class_id, index);

  if (o->members[index].code==9 && o->members[index].data.o->class_id==1829)
    subclass = o->members[index-1].data.si;
  if (o->members[index].code==9 && o->members[index].data.o->class_id==1830)
    printf("<tr id=\"t%d_%d\">", subclass, o->members[index].data.o->members[1].data.si);
  else
    printf("<tr>");
  for (int i=0; i<9; i++)
  { printf("<td align=right>");
    if (i == indent)
      printf("%d", index);
    printf("</td>");
  }
  if (pindex<0 && c && c->name)
    printf("<td>%s</td>", c->name);
  else
    printf("<td></td>");
  printf("<td>%s</td>", codenames[o->members[index].code]);
  if (val)
  { if (c && c->type && !strcmp(c->type, "pstring")) 
      printf("<td><pre>%s</pre></td>", val);
    else
      printf("<td>%s</td>", val);
  }
  else
    printf("<td></td>");
  printf("</tr>\n"); 
}

static char *linkid(int id, int key)
{ static char val[128];
  sprintf(val, "\"telaradb.cgi?id=%d&key=%d\"", id, key);
  return val;
}

static char *disclink(char *cat, char *val, member *m)
{ char *name = discname(cat, m->data.si, 0);
  if (name)
    sprintf(val, "<a href=\"discovery.cgi?cat=%s&id=%d\">%s</a>", cat, m->data.si, name);
    else
      sprintf(val, "<span style=\"color:#FF0000;\">%d / 0x%08x / %g</span>", m->data.si,  m->data.ui, m->data.f);
}

static void print(obj *o, int index, int indent, int pindex)
{ int s;
  member *m;

  if (indent >=0)
  { m = o->members+index;
    s = m->type;
  }
  else
    s = T_CLASS;

  conf *c;
  if (pindex >= 0)
    c = getconf(o->class_id, pindex);
  else
    c = getconf(o->class_id, index);

  switch(s)
  { case T_NUM:
    { char val[128];
      if (c && c->istring)
      { m->code = 17;
        sprintf(val, "<a href=%s>%s</a>", linkid(c->istring, m->data.si), getname(c->istring, m->data.si));
      }
      else if (c && c->listring)
      { m->code = 18;
        sprintf(val, "<a href=%s>%s</a>", linkid(c->listring, m->data.si), getname(c->listring, m->data.si));
      }
      else if (c && c->link)
      { m->code = 19;
	dbkey k;
	k.id = c->link;
        k.key = m->data.si;
	if (c->linkt)
	  k = getkey(c->type, c->linkt, m->data.si);
        char *name=getname(k.id, k.key);
        if (!name)
        {
          sprintf(val, "<a href=%s style=\"color:#FF0000;\">%d</a>", linkid(k.id, k.key), k.key);
        }
	else
        { if (!strlen(name))
	  { name = calloc(32, 1);
            sprintf(name, "%d", m->data.si);
          }
          sprintf(val, "<a href=%s>%s</a>", linkid(k.id, k.key), name);
        }
      }
      else
      { if (c && c->type && !strcmp(c->type, "float"))
        { m->code = 20;
	  sprintf(val, "%g", m->data.f);
	}
	else if (c && c->type && !strcmp(c->type, "double"))
	{ m->code = 28;
	  sprintf(val, "%lg", m->data.d);
	}
	else if (c && c->type && !strcmp(c->type, "int"))
	{ m->code = 21;
	  sprintf(val, "%ld", m->data.si);
	}
	else if (c && c->type && !strcmp(c->type, "nlstring"))
	{ m->code = 29;
	  sprintf(val, "%s", nlstring(m->data.si));
	}
	else if (c && c->type && !strcmp(c->type, "item"))
	{ m->code = 22;
          disclink("Item", val, m);
        }
	else if (c && c->type && !strcmp(c->type, "quest"))
	{ m->code = 23;
          disclink("Quest", val, m);
        }
	else if (c && c->type && !strcmp(c->type, "NPC"))
        { m->code = 24;
          disclink("NPC", val, m);
        }
        else if (c && c->type && !strcmp(c->type, "recipe"))
        { m->code = 25;
          disclink("Recipe", val, m);
        }
	else if (c && c->type && !strcmp(c->type, "wintime"))
        { m->code = 26;
	  time_t time = m->data.ui/10000000-11644473600;
	  sprintf(val, "%s", ctime(&time));
        }
	else if (c && c->type && !strcmp(c->type, "talent"))
        { m->code = 19;
          int classid = o->members[index-1].data.si;
          int talent = m->data.si;

	  tps_member *tm = tpath_obj("#1827:/.0/{%d}/.0/@1830[.1=%d]/.2", classid, talent);
	  char *tname = lstring(tm->m.data.o);
	  sprintf(val, "<a href=\"telaradb.cgi?id=1827&key=%d#t%d_%d\">%s</a>", tm->key, classid, talent, tname);
	}
        else if (c && c->type && !strcmp(c->type, "talent2"))
        { m->code = 19;
          int talent = m->data.si;
          int classid = o->members[index+1].data.si;

          tps_member *tm = tpath_obj("#1827:/.0/{%d}/.0/@1830[.1=%d]/.2", classid, talent);
          char *tname = lstring(tm->m.data.o);
          sprintf(val, "<a href=\"telaradb.cgi?id=1827&key=%d#t%d_%d\">%s</a>", tm->key, classid, talent, tname);
        }
        else
        { if (m->code < 2)	// boolean
	    sprintf(val, "%d", m->data.ui);
          else 
	    if (m->data.ui & ~(unsigned long)0xffffffff)
	      sprintf(val, "%ld / 0x%016lx / %lg", m->data.si, m->data.ui, m->data.d);
	    else
              sprintf(val, "%d / 0x%08x / %g", m->data.si, m->data.ui, m->data.f);
        }
      }
      output(o, index, indent, val, pindex);
      break;
    }

    case T_STRING: 
	if (c && c->type && !strcmp(c->type, "idstring"))
        { char val[1024], link[128];
          char *part=strtok(m->data.s, "#");
	  strcpy(val, part);
	  while (part=strtok(NULL, "#"))
	  { sprintf(link, "#<A href=%s>%s</A>", linkid(c->link, atoi(part)), part);
	    strcat(val, link);
	  }
	  output(o, index, indent, val, pindex);
	}
	else
	  output(o, index, indent, m->data.s, pindex);
	break;

    case T_CLASS:
      if (indent >= 0)
      { int id = m->data.o->class_id;
        if (id == 7703)
        { m->code = 16;
          output(o, index, indent, lstring(m->data.o), pindex);
          break;
        }
	else if (id==51)
	{ m->code=27;
	  obj *o2 = m->data.o;
          char val[128];
          float r, g, b, a;

          r = o2->members[0].data.f;
          g = o2->members[1].data.f;
          b = o2->members[2].data.f;
	  a = o2->members[3].data.f;

          sprintf(val, "<span style=\"background-color:#%02x%02x%02x;\">&nbsp;&nbsp;&nbsp;&nbsp;</span> %g / %g / %g alpha %g", (int)r, (int)g, (int)b, r, g, b, a);

	  output(o, index, indent, val, pindex);
	  break;
	}
	else if (id==314)
	{ m->code=27;
	  obj *o2 = m->data.o->members[0].data.o;
          char val[128];
          float r, g, b, a;

          r = o2->members[0].data.f;
          g = o2->members[1].data.f;
          b = o2->members[2].data.f;
          a = o2->members[3].data.f;

          sprintf(val, "<span style=\"background-color:#%02x%02x%02x;\">&nbsp;&nbsp;&nbsp;&nbsp;</span> %g / %g / %g alpha %g", (int)(255*r), (int)(255*g), (int)(255*b), r, g, b, a);

          output(o, index, indent, val, pindex);
          break;
	}
        else
        { static char val[32];
	  sprintf(val, "classid %d", m->data.o->class_id);
          output(o, index, indent, val, -(pindex<0));
	  o = m->data.o;
        }
      }
      for (int i=0; i<o->nmemb; i++)
        print(o, i, indent+1, -1);
      break;

    case T_ARRAY:
    { obj *o2 = m->data.o;
      conf *c = getconf(o->class_id, index);
      if (c && c->type && !strcmp(c->type, "color"))
      { m->code=27;
	char val[128];
        float r, g, b;

        r = o2->members[0].data.f;
        g = o2->members[1].data.f;
        b = o2->members[2].data.f;

	if (r>1 || g>1 || b>1)
	  sprintf(val, " %g / %g / %g", r, g, b);
	else
          sprintf(val, "<span style=\"background-color:#%02x%02x%02x;\">&nbsp;&nbsp;&nbsp;&nbsp;</span> %g / %g / %g", (int)(255*r), (int)(255*g), (int)(255*b), r, g, b);
	if (o2->nmemb>3)
	{ char *val2 = strdup(val);
          float a = o2->members[3].data.f;
	  sprintf(val, "%s alpha %g", val2, a);
	}
	output(o, index, indent, val, -1);
      }
      else
      { output(o, index, indent, "", -1);
        o2->class_id = o->class_id;
        for (int i=0; i<o2->nmemb; i++)
          print(o2, i, indent+1, index);
      }
      break;
    }

    case T_MAP:
    { output(o, index, indent, "", -1);
      obj *o2 = m->data.o;
      o2->class_id = o->class_id;
      for (int i=0; i<o2->nmemb; i++)
        print(o2, i, indent+1, index);
      break;
    }
  } 
}

static void tpath_query(char *path)
{ char *remoteip = getenv("REMOTE_ADDR");
  if (!remoteip || strcmp(remoteip, SECURE_IP))
  { printf("Connection from unsecure IP - see common.h\n");
    return;
  }
  
  tset *set = tpath_set(path);
  if (tpath_errno)
  { printf("Tpath error %d: %s\n", tpath_errno, tpath_errmsg);
    return;
  }
  if (!set->nmemb)
  { printf("Tpath: no match found\n");
    return;
  }
  printf("Tpath (%s) result:\n", path);
  for (int i=0; i<set->nmemb; i++)
  { obj *o = tpath_tm2obj(set->members+i);

    printf("<table>\n");
    printf("<tr><th colspan=9>Index</th><th>Name</th><th>Type</th><th>Value</th></tr>\n");

    print(o, 0, -1, -1);

    printf("</table>\n");
  }
  printf("</div></body></html>\n");
}

static int id;

static void cb(int key)
{ char *name=getname(id, key);
  if (!name || !strlen(name))
    printf(" / <a href=%s>%d</a>\n", linkid(id, key), key);
  else
    printf(" / <a href=%s>%s</a>\n", linkid(id, key), name);
}

char *release;

int main(int argc, char **argv)
{ char *query;
  release = "current";

  query = getenv("QUERY_STRING");

  int key=0, all=0, class=0;
  char *skey="", *sid="";
  char *tpath = NULL;
  char *arg=strtok(query, "&");
  do
  { if (!strncmp(arg, "id=", 3))
      sid=arg+3;
    else if (!strncmp(arg, "key=", 4))
      skey=arg+4;
    else if (!strncmp(arg, "class=", 6)) 
      class = atoi(arg+6);
    else if (!strncmp(arg, "rel=", 4))
      release = RELEASE2;
    else if (!strncmp(arg, "tpath=", 6))
      tpath = url_decode(arg+6);
  } while (arg=strtok(NULL, "&"));

  html_header("telaradb", sid, skey, tpath?tpath:"");

  id = atoi(sid);
  if (!strcmp(skey, "all"))
    all = 1;
  else
    key = atoi(skey);

  if (tpath)
  { tpath_query(tpath);
    return 0;
  }

  if (class)
  { int c;
    char line[1024], cline[1024];
    FILE *ci = fopen(CONF_DIR "class.index", "r");
    while (fgets(line, 1023, ci))
    { sscanf(line, "%d %1023s", &c, cline);
      if (c == class)
      { int s=strlen(cline);
        char *num = strtok(cline, ",");
        if (strlen(num) != s)
        { printf("class %d found in the following datasets: <a href=\"telaradb.cgi?id=%d&key=all\">%d</a>", class, atoi(num), atoi(num));
          while (num=strtok(NULL, ","))
            printf(", <a href=\"telaradb.cgi?id=%d&key=all\">%d</a>", atoi(num), atoi(num));
	  printf("\n</div></body></html>\n");
          return 0;
	}
        id = atoi(cline);
	all = 1;
	break;
      } 
    }
    fclose(ci);
    if (!id)
    { printf("No such (toplevel) class\n");
      return 0;
    }
  }

  conf *c = getconf(id, -1);
  if (c && c->name)
    printf("<b>%s</b>  ", c->name);
  if (all)
  { printf("\nid: %d<br>\n", id);
    disable_obj_chache();
    read_all(id, &cb);
  }
  else if (id)
  { printf("id: %d, key: %d ", id, key);

    obj *o = read_obj(id, key);
    if (!o)
    { printf("No such id/key\n");
      return 0;
    } 

    if (o->akey1 || o->akey2 || o->name)
    { printf(" (");
      if (o->akey1)
        printf(" akey1: %s", o->akey1);
      if (o->akey2)
        printf(" akey2: %s", o->akey2);
      if (o->name)
        printf(" name: %s", o->name);
      printf(" )\n");
    }
 
    printf("classid %d\n", o->class_id);
    char *cat=NULL, *mcat=NULL;
    switch(id)
    { case 113:
	mcat = "zone";
	break;

      case 2200:
        cat = "Achievement";
	mcat = "achievement";
        break;

      case 4201:
	mcat = "faction";
	break;

      case 10701:
        cat = "ArtifactCollection";
	mcat = "artifactset";
        break;
    }
    if (cat)
    { if (discname(cat, key, 0))
        printf("<a href=\"discovery.cgi?cat=%s&id=%d\">%ss.xml</a>\n", cat, key, cat);
      else
        printf("<a style=\"color:#FF0000;\" href=\"discovery.cgi?cat=%s&id=%d\">%ss.xml</a>\n", cat, key, cat);
    }

    if (id == 7629)
      printf(" <a href=\"" TELARADB "appearances/%d\">telaradb.com</a>\n", tpath_obj("#7629/%d:/.1/.0", key)->m.data.si);

    if (mcat)
      printf(" <a href=\"" MAGELO "%s/%d\">magelo.com</a>\n", mcat, key);

    printf("<table>\n");
    printf("<tr><th colspan=9>Index</th><th>Name</th><th>Type</th><th>Value</th></tr>\n");

    print(o, 0, -1, -1);

    printf("</table>\n");
  }
  printf("</div></body></html>\n");

  return 0;
}
