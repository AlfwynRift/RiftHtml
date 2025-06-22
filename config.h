
typedef struct conf
{ char *name;
  int istring;
  int listring;
  int link;
  int linkt;
  char *type;
} conf;

conf *getconf(int id, int index);
int getindex(int id, char *name);
