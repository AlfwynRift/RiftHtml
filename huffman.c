#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <malloc.h>
#include <unistd.h>
#include "common.h"
#include "huffman.h"

static int heapl = 512;

static node *maketable(int *values)
{
            int nextDepth;
            int result;
            int maxDepth = 9;
            int v9;
            int v10;
            int v12[32];	bzero(v12, 32*sizeof(int));
            int a3[512];	bzero(a3, 512*sizeof(int));
            int codeValues[512];bzero(codeValues, 512*sizeof(int));
            int codeLength[512];bzero(codeLength, 512*sizeof(int));
            int depth = 0;
            int v6 = 0;

            v12[0] = 1;
            do
            {
                result = values[(v6 & 1) * 256 + v12[depth]];
                if (result < 0)
                {
                    nextDepth = depth + 1;
                    do
                    {
                        if (nextDepth == maxDepth)
                            a3[v6] = result;
                        v6 *= 2;
                        ++depth;
                        v12[depth] = -result;
                        result = values[-result];
                        ++nextDepth;
                    } while (result < 0);
                }
                codeValues[result] = v6;
                codeLength[result] = depth + 1;
                if (depth + 1 <= maxDepth)
                {
                    if (depth + 1 == maxDepth)
                    {
                        a3[v6] = result;
                    }
                    else
                    {
                        v9 = maxDepth - (depth + 1);
                        v10 = 1 << v9;
                        if (1 << v9 != 0)
                        {
                            int v11 = v10 + (v6 << v9);
                            do
                            {
                                --v10;
                                --v11;
                                v11 = result;
                                a3[v11] = result;
                            } while (v10 != 0);
                        }
                    }
                }
                do
                {
                    if ((v6 & 1) == 0)
                        break;
                    v6 >>= 1;
                    --depth;
                } while (depth >= 0);
                v6 |= 1;
            } while (depth >= 0);

            node *root = (node *)calloc(1, sizeof(node));

            for (int i = 0; i < 256; ++i)
            {
                if (codeLength[i] != 0)
                {
                    int mask = 1 << (codeLength[i] - 1);

                    node *current = root;
                    while (mask != 0)
                    {
                        int b = (codeValues[i] & mask) != 0;
                        if (b)
                        { if (current->b == NULL)
                            {
                                current->b = (node *)calloc(1, sizeof(node));
                            }
                            current = current->b;
                        }
                        else
                        { if (current->a == NULL)
                            {
                                current->a = (node *)calloc(1, sizeof(node));
                            }
                            current = current->a;
                        }
                        mask = (int)((uint)mask >> 1);
                    }
                    current->value = i;

                }
            }
            return root;
}

static void heapify(int *heap, int insert, int weight, int value, int limit)
{ 
            for (int j = insert << 1; ; j <<= 1)
            {
                if (j > limit)
                    break;

                if (j < limit)
                {
                    if (heap[j - 1 + (heapl >> 1)] > heap[j + (heapl >> 1)])
                    {
                        j++;
                    }
                }
                if (weight <= heap[j - 1 + (heapl >> 1)])
                    break;

                heap[insert - 1] = heap[j - 1];
                heap[insert - 1 + (heapl >> 1)] = heap[j - 1 + (heapl >> 1)];
                insert = j;
            }

            heap[insert - 1] = value;
            heap[insert - 1 + (heapl >> 1)] = weight;
}

node *buildTree(uint32 *frequencies)
{
            // The heap is twice the size of the array as we will first store pairs inside
            int *heap = (int *)calloc(heapl,sizeof(int));
            for (int i = 0; i < 256; ++i)
            {
                heap[i] = i;
                heap[i + (heapl >> 1)] = frequencies[i];
            }

            for (int i = (heapl >> 2); i > 0; --i)
            {
                int value = heap[i - 1];
                int weight = heap[i - 1 + (heapl >> 1)];

                heapify(heap, i, weight, value, (heapl >> 1));
            }

            int prevFreq = 0;

            int b = 0;
            int limit = heapl >> 1;
            while (1)
            {
                b++;
                if ((b & 1) == 0)
                {
                    int value = -limit;
                    int weight = prevFreq + heap[0 + (heapl >> 1)];

                    heap[limit + (heapl >> 1)] = heap[0];

                    heapify(heap, 1, weight, value, limit);
                }
                else
                {
                    limit--;
                    int value = heap[limit];
                    int weight = heap[limit + (heapl>> 1)];

                    // Copy the first value of the heap as a node of the inline tree
                    heap[limit] = heap[0];

                    if (limit == 1)
                    {
                        heap[1 + (heapl >> 1)] = value;
                        break;
                    }

                    prevFreq = heap[0 + (heapl >> 1)];

                    heapify(heap, 1, weight, value, limit);
                }
            }
           return  maketable(heap);
}

unsigned long leb128(byte **in)
{ unsigned long ret=0;
  byte *input = *in;
  unsigned sh=0;
 
  do
  { ret += ((long)(*input&127))<<sh;
    sh += 7;
  } while (*input++ >= 128); 

  *in = input;
  return ret;
}

byte *decompress(byte *input, int olen, uint32 *freq, node *tree)
{ node *root;

  if (freq)
    root = buildTree(freq);
  else
    root = tree;

  node *cur = root;
  byte *output = (byte *)malloc(olen);
 
  for (int i=0; i<olen; input++)
  { byte b = *input;

    for (byte o=0x80; o && i<olen; o>>=1)
    { if (b&o)
        cur = cur->b;
      else
        cur = cur->a;
      if (!cur->a)
      { output[i++] = cur->value;
        cur = root;
      }
    }
  }

  return output;
}
