
typedef struct obj obj;

typedef struct member
{ int code;
  enum type
  { T_NONE,
    T_NUM,
    T_STRING,
    T_COMP,
    T_ARRAY,
    T_MAP,
  } type;
  union data
  { signed long si;
    unsigned long ui;
    float f;
    double d;
    char *s;
    obj *o;
  } data;
} member;

struct obj
{ int class_id;
  char *akey1, *akey2;
  char *name;
  int nmemb;
  member members[];
};

void disable_obj_chache(void);
obj *decode_obj(byte *data, int dlen);
obj *read_obj(int id, int key);
char *getname(int id, int key);
void read_all(int id, void (*callback)(int));

typedef struct dbkey
{ int id;
  int key;
} dbkey;

dbkey getkey(char *idlist, int ktype, int key);
