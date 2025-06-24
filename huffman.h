
typedef struct node node;
struct node
{ int value;
  node *a, *b;
};

unsigned long leb128(byte **in);
node *buildTree(uint32 *frequencies);
byte *decompress(byte *input, int olen, uint32 *freq, node *tree);
