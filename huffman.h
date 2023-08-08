/* Copyright (c) 2013 the authors listed at the following URL, and/or
the authors of referenced articles or incorporated external code:
http://en.literateprograms.org/Huffman_coding_(C_Plus_Plus)?action=history&offset=20090129100015

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Retrieved from: http://en.literateprograms.org/Huffman_coding_(C_Plus_Plus)?oldid=16057
*/

#ifndef HUFFMAN_H_INC
#define HUFFMAN_H_INC
#define BINARY_CODES

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include "bitstream.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef uint32_t DataType;
typedef int Frequency;
struct Hufftree;
typedef struct Hufftree Hufftree;

Hufftree* new_Hufftree(Frequency *frequency, int size);
Hufftree*  new_Hufftree2(GetBitContext *s);
size_t encodetree_Hufmantree( Hufftree*h, PutBitContext *s);
DataType decode_Hufftree(Hufftree*h, GetBitContext *s);
void encode_Hufftree(Hufftree*h, DataType v, PutBitContext *s);

void delete_Hufftree(Hufftree* hufftree);
void delete_Hufftreep(Hufftree** hufftree);


#ifdef __cplusplus
}
#endif


#endif