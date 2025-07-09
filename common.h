#define	BASE_DIR 	"../"
#define DATA_DIR	BASE_DIR "data/"
#define CONF_DIR	BASE_DIR "conf/"
#define DISCOVERY_DIR   DATA_DIR

#define RELEASE		"STABLE-304-1083-A-1253495"
#define RELEASE2	"STABLE-1-1091-A-1255760"

extern char *release;

/*
  Optionally define a secure IP adress, that is allowed to make tpath queries
  See: https://thegharstation.com/gharwiki/index.php/User:Alfwyn/Sandbox#Tpath
  Currently only a limited subset is implemented, but more is planned.
  This feature mainly exists for me to debug tpath expressions and the tpath
  implementation.

  No attempt is made to sanitize the tpath string, and malicious
  expressions could be a real resource hog.

  127.0.0.1 may be a good choice for the definition.
*/
#define SECURE_IP       "x"


#include <stdint.h>
typedef unsigned char byte;

typedef uint16_t uint16;
typedef uint32_t uint32;

typedef int16_t int16;
typedef int32_t int32;
