#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sqlite3.h>
#include "common.h"
#include "huffman.h"
#include "database.h"
#include "config.h"
#include "lstring.h"

typedef struct entry
{ unsigned int code;
  unsigned long data;
} entry;

typedef struct entry2
{ unsigned int code1;
  unsigned int code2;
  unsigned long data;
} entry2;

static int freq[256];
static node *tree = NULL;

static entry split(unsigned long long in)
{ entry ret;

  int code = in&7;
  if (code == 7)
  { ret.code = 7+((in>>3)&7);
    ret.data = in >> 6;
  }
  else
  { ret.code = code;
    ret.data = in >> 3;
  }

  return ret;
}

static entry2 split2(unsigned long long in)
{ entry2 ret;

  entry h = split(in);
  ret.code1 = h.code;
  h = split(h.data);
  ret.code2 = h.code;
  ret.data = h.data;

  return ret;
}

static int ftable(void *unused, int argc, char **argv, char **col_name)
{
  memcpy(freq, argv[0], 1024);
  return 0;
}

static byte *data;
static byte *decode(byte *d, obj *o, entry e);
static obj *o_ret = NULL;

obj *decode_obj(byte *data, int dlen)
{ entry e;
  e.code = 9;
  byte *d = decode(data, NULL, e);
  if (d != data+dlen)
  { printf("data mismatch:\td: %p\tdata+dlen: %p\n", d, data+dlen);
    exit(0);
  }
  return o_ret;
}

static int dataset(void *unused, int argc, char **argv, char **col_name)
{ byte *p = argv[3];
  int dlen = leb128(&p);
  if (!tree)
    tree = buildTree(freq);
  data = decompress(p, dlen, NULL, tree);
  decode_obj(data, dlen);
  if (argv[0])
    o_ret->akey1 = strdup(argv[0]);
  else
    o_ret->akey1 = NULL;
  if (argv[1])
    o_ret->akey2 = strdup(argv[1]);
  else
    o_ret->akey2 = NULL;
  if (argv[2])
    o_ret->name = strdup(argv[2]);
  else
    o_ret->name = NULL;
  return 0;
}

static void (*cb)(int);
static int datakey(void *unused, int argc, char **argv, char **col_name)
{ 
  (*cb)(atoi(argv[0]));
  return 0;
}

static sqlite3 *db = NULL;

static void open_db(void)
{ char filename[128];

  if (db)
    return;

  snprintf(filename, 127, "%s%s/telara.db3", DATA_DIR,  release);

  if (sqlite3_open(filename, &db))
  { printf("SQL error: %s\n", sqlite3_errmsg(db));
    printf("%s", filename);
    exit(0);
  }
}

static byte *decode(byte *d, obj *o, entry e)
{ 
  switch(e.code)
  { case 0:
       o->members[e.data].type = T_NUM;
       o->members[e.data].data.ui = 0;
       break;

    case 1:
       o->members[e.data].type = T_NUM;
       o->members[e.data].data.ui = 1;
       break;

    case 2:
      o->members[e.data].type = T_NUM;
      o->members[e.data].data.ui = leb128(&d);
      break;

    case 3: // zigzag
    { o->members[e.data].type = T_NUM;
      long v = (long)leb128(&d);
      v = v/2 ^ -(v&1);
      o->members[e.data].data.si = v;
      break;
    }

    case 4:
      o->members[e.data].type = T_NUM;
      o->members[e.data].data.ui = (d[0] + 256*d[1] + 65536*d[2] + 16777216*d[3])&(unsigned long)0xffffffff;
      d += 4;
      break;

    case 5:
      o->members[e.data].type = T_NUM;
      o->members[e.data].data.ui = d[0] + 256*d[1] + 65536*d[2] + 16777216*d[3]
	  + (((unsigned long)d[4] + 256*d[5] + 65536*d[6] + 16777216*d[7])<<32);
      d += 8;
      break;

    case 6:
    { int len = leb128(&d);
      o->members[e.data].type = T_STRING;
      o->members[e.data].data.s = calloc(len+1, 1); // null terminate
      memcpy(o->members[e.data].data.s, d, len);
      d += len;
      break;
    }

    case 9: 	// class
    { entry e2;
      obj *o2 = calloc(1, sizeof(obj)+256*sizeof(member));
      o2->nmemb = 256;
      o2->class_id = leb128(&d);
      if (o)
      { o->members[e.data].type = T_CLASS;
        o->members[e.data].data.o = o2;
      }
      else
	o_ret = o2;
      while ((e2 = split(leb128(&d))).code != 7)
      { o2->members[e2.data].code = e2.code;
	d = decode(d, o2, e2);
      }
      break;
    }

    case 10:	// array
    { entry h=split(leb128(&d));
      int size = h.data;
      obj *o2 = calloc(1, sizeof(obj)+size*sizeof(member));
      o->members[e.data].type = T_ARRAY;
      o->members[e.data].data.o = o2;
      o2->nmemb = size;
      for (int i=0; i<size; i++)
      { o2->members[i].code = h.code;
        h.data = i;
        d = decode(d, o2, h);
      }
      break;
    }

    case 11:	// map
    { entry2 h = split2(leb128(&d));
      int size = h.data;
      obj *o2 = calloc(1, sizeof(obj)+2*size*sizeof(member));
      o->members[e.data].type = T_MAP;
      o->members[e.data].data.o = o2;
      o2->nmemb = 2*size;
      for (int i=0; i<size; i++)
      { o2->members[2*i].code = h.code1;
	e.code = h.code1;
	e.data = 2*i;
        d = decode(d, o2, e);
	o2->members[2*i+1].code = h.code2;
	e.code = h.code2;
	e.data = 2*i + 1;
	d = decode(d, o2, e);
      }
      break;
    }
  }

  return d;
}

typedef struct ocline
{ int id;
  int key;
  obj *o;
} ocline;

static ocline *ocache = NULL;
static int oclen = 0;
static int nocache = 0;

void disable_obj_chache(void)
{ nocache = 1;
}

static obj *obj_cache(int id, int key)
{ for (int i=0; i<oclen; i++)
  { if (id==ocache[i].id && key==ocache[i].key)
      return ocache[i].o;
  }
  return NULL;
}

static void obj_add2cache(int id, int key, obj *o)
{ static int maxlen = 0;

  if (nocache)
    return;

  if (oclen == maxlen)
  { maxlen += 32;
    ocache = realloc(ocache, maxlen*sizeof(ocline));
  }
  ocache[oclen].id = id;
  ocache[oclen].key = key;
  ocache[oclen].o = o;
  oclen++;
}

obj *read_obj(int id, int key)
{ char sql[256];
  static int lastid = 0;

  open_db();

  obj *c = obj_cache(id, key);
  if (c)
    return c;

  if (id != lastid)
  { snprintf(sql, 255, "SELECT frequencies FROM dataset_compression WHERE datasetId=%d", id);
    if (sqlite3_exec(db, sql, ftable, NULL, NULL))
    { printf("SQL error: %s\n", sqlite3_errmsg(db));
      exit(0);
    }
    tree = NULL;
  }
  lastid = id;

  o_ret = NULL;
  snprintf(sql, 255, "SELECT alternateKey1, alternateKey2, name, value FROM dataset WHERE datasetId=%d AND datasetKey=%d", id, key);
  if (sqlite3_exec(db, sql, dataset, NULL, NULL))
  { printf("SQL error: %s\n", sqlite3_errmsg(db));
    exit(0);
  }
  obj_add2cache(id, key, o_ret);
  return o_ret;
}

char *getname(int id, int key)
{ static int lastid=0, noname=0;

  if (id==lastid && noname)
    return "";
  lastid = id;

  obj *o=read_obj(id, key);
  if (!o)
    return NULL;

  int i=getindex(o->class_id, "Name");
  if (i<0)
    i=getindex(o->class_id, "iName");
  if (i<0)
  { noname = 1;
    return "";
  }
  else
    noname = 0;

  if (o->members[i].type == T_NONE)
  { i=getindex(o->class_id, "iName");
    if (i<0)
      return "";
  }
  if (o->members[i].type == T_STRING)
    return o->members[i].data.s;
  else if (o->members[i].type == T_CLASS && o->members[i].data.o->class_id == 7703)
  { char *ret = lstring(o->members[i].data.o);
    if (strcmp(" ", ret))
      return lstring(o->members[i].data.o);
    else
      return "";
  }
  else if (o->members[i].type == T_NUM)
    return nlstring(o->members[i].data.si);
  else
    return "";
}

void read_all(int id, void (*callback)(int))
{ char sql[256];

  cb = callback;
  open_db();
  snprintf(sql, 255, "SELECT datasetKey FROM dataset WHERE datasetId=%d", id);
  if (sqlite3_exec(db, sql, datakey, NULL, NULL))
  { printf("SQL error: %s\n", sqlite3_errmsg(db));
    exit(0);
  }
  return;
}

static dbkey ret_keyval;
static int keyval(void *unused, int argc, char **argv, char **col_name)
{ ret_keyval.id = atoi(argv[0]);
  ret_keyval.key = atoi(argv[1]);
  return 0;
}

dbkey getkey(char *idlist, int ktype, int key)
{ char sql[256];

  char *kn;
  switch (ktype)
  { case 1:
      kn = "alternateKey1";
      break;

    case 2:
      kn = "alternateKey2";
      break;
  }

  open_db();

  ret_keyval.id = 0;
  snprintf(sql, 255, "SELECT datasetId, datasetKey FROM dataset WHERE datasetId IN (%s) AND %s=%d", idlist, kn, key);
  if (sqlite3_exec(db, sql, keyval, NULL, NULL))
  { printf("SQL error: %s\n", sqlite3_errmsg(db));
    exit(0);
  }
  return ret_keyval;
}
