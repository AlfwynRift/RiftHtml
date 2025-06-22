#include <stdint.h>

typedef unsigned char byte;

typedef uint16_t uint16;
typedef uint32_t uint32;

typedef int16_t int16;
typedef int32_t int32;

typedef unsigned char uint8;
typedef signed char int8;

typedef struct node node;
struct node
{ int value;
  node *a, *b;
};

unsigned long leb128(byte **in);
node *buildTree(uint32 *frequencies);
byte *decompress(byte *input, int olen, uint32 *freq, node *tree);
