typedef struct dconf
{ char *cat;
  int link;
} dconf;

dconf *getdconf(char *class, char *tag);
