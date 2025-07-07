
extern int tpath_errno;
extern char *tpath_errmsg;

/* "inherit" from member */
typedef struct tps_member
{ member m;
  int key;
} tps_member;

/* convert tps_member back to base object member */
#define	tpath_member(tpsm)	(*(member *)&(tpsm))

typedef struct tset
{ int nmemb;
  tps_member members[];
} tset;

tset *tpath_set(const char *path, ...);
tps_member *tpath_obj(const char *path, ...);
obj *tpath_tm2obj(tps_member *tm);
