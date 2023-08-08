#include "huffman.h"
#include <assert.h>
#include <stdio.h>

struct Node;
typedef struct Node Node;
struct SEncoding;
typedef struct SEncoding SEncoding;


struct SEncoding {
   uint32_t code;
   int      bit_length;
};



struct Hufftree
{
  Node* tree;
  int size;     //alphabet size
  int symbols;  //symbols (freq > 0);
  SEncoding* encoding;
  Node*nodesPool_heap;
};


struct Node
{
   Node* leftChild;
   Node* rightChild; // if leftChild != 0
   Frequency frequency;
   DataType data;  // if leftChild == 0
};


static int ilog2_32(uint32_t v)
{
   if (!v)
      return 0;

   return 32-__builtin_clz(v);
}


  static Node* init_Node(Frequency f, DataType d, Node* n) {
      n->frequency = f;
      n->data = d;
      return n;
  }



  static Node* init_Node2(Node* left, Node* right, Node*n) {
    n->frequency = left->frequency + right->frequency;
    n->data = 0;
    n->leftChild = left;
    n->rightChild = right;
    return n;
  }


static void fill_Node(Node *n, Hufftree* h,uint32_t prefix, int bit_length)
  {
    if (n->leftChild)
    {
      fill_Node(n->leftChild,  h, prefix<<1, bit_length+1);
      fill_Node(n->rightChild, h, prefix<<1|1, bit_length+1);
    }
    else {
      h->encoding[n->data].bit_length = bit_length;
      h->encoding[n->data].code = prefix;
    }
}

void delete_Hufftree(Hufftree* hufftree) {
   if (hufftree) {
      free(hufftree->encoding);
      free(hufftree->nodesPool_heap);
      free(hufftree);
   }
}

void delete_Hufftreep(Hufftree** hufftree) {
   delete_Hufftree(*hufftree);
   *hufftree=NULL;
}

static int compare(const void*a, const void *b) {
    Node* aa = *(Node**) a;
    Node* bb = *(Node**) b;
    assert (aa != bb);
    return aa->frequency - bb->frequency;
}

Hufftree* new_Hufftree(Frequency *frequency, int size)
{
   Hufftree *hufftree = (Hufftree *)calloc(1, sizeof(Hufftree));
   Node* nodespool = hufftree->nodesPool_heap = (Node*) calloc(size*2, sizeof(Node));
   assert(hufftree);
   hufftree->encoding = calloc(size,sizeof(SEncoding));
   assert(hufftree->encoding);
   hufftree->size = size;
   Node** pqueue = malloc(size*sizeof(Node*));
   assert(pqueue);
   int pqueue_sz = 0;
   for (int i=0; i<size; i++) {
      if (frequency[i])
         pqueue[pqueue_sz++] = init_Node(frequency[i], i, nodespool++);
   }

   Node** pqueue_p = pqueue;
   int nb_nodes=pqueue_sz;
   hufftree->symbols = nb_nodes;
   assert( pqueue_sz > 1 );
   while (pqueue_sz)
   {
      if (pqueue_sz >= 2) {
         qsort(pqueue_p,pqueue_sz,sizeof(*pqueue_p), compare);
      }

      Node* top = pqueue_p[0];
      Node* top2 = pqueue_sz >= 2 ? pqueue_p[1] : NULL;
      pqueue_p[0] = NULL;

      if (top2 == NULL)
      {
         hufftree->tree = top;
         break;
      }
      pqueue_p++; pqueue_sz--;
      pqueue_p[0] = init_Node2(top,top2,nodespool++);
   }
   free(pqueue);
   assert(hufftree->tree);
   fill_Node(hufftree->tree, hufftree, 0, 0);
   return hufftree;
}


void encode_Hufftree(Hufftree*h, DataType v, PutBitContext *s)
{
   assert(v < h->size );
   int bit_length = h->encoding[ v ].bit_length;
   put_bits(s, h->encoding[ v ].bit_length, h->encoding[ v ].code);
}


DataType decode_Hufftree(Hufftree*h, GetBitContext *s)
{
   Node* node = h->tree;
   for(;;){
      node =  get_bits1(s) ? node->rightChild : node->leftChild;
      if (!node->leftChild)
      {
         return node->data;
      }
   }
}

struct __pack_ctx_tag {

    PutBitContext *s;
    size_t bit_length;
    int bitwidth;

};

   void __pack(Node*n, struct __pack_ctx_tag *ctx ) {
        if (n->leftChild) {
            put_bits(ctx->s,1,0);
            __pack(n->leftChild,ctx);
            __pack(n->rightChild,ctx);
        } else {
            put_bits(ctx->s,1,1);
            put_bits(ctx->s,ctx->bitwidth,n->data);
            ctx->bit_length+=ctx->bitwidth;
        }
        ctx->bit_length++;
   }


size_t encodetree_Hufmantree( Hufftree*h, PutBitContext *s)
{
    struct __pack_ctx_tag ctx;
    ctx.bitwidth = ilog2_32( h->size-1 );
    put_bits(s,5,ctx.bitwidth);
    ctx.s = s;
    ctx.bit_length = 0;
   __pack(h->tree,&ctx);
   return ctx.bit_length;
}


struct __unpack_ctx_tag {

    Hufftree *hufftree;
    int bitsize;
    GetBitContext *s;
    Node* nodesPool;

};


Node* __unpack(struct __unpack_ctx_tag *ctx) {
    if (get_bits1(ctx->s)) {
        ctx->hufftree->symbols++;
        return init_Node(0,get_bits(ctx->s,ctx->bitsize),ctx->nodesPool++);
    }
    else {
        Node* left = __unpack(ctx);
        Node* right = __unpack(ctx);
        return init_Node2( left, right, ctx->nodesPool++ );
    }
}


Hufftree*  new_Hufftree2(GetBitContext *s)
{
   struct __unpack_ctx_tag ctx;
   ctx.hufftree = (Hufftree *)calloc(1, sizeof(Hufftree));
   ctx.bitsize = get_bits(s,5);
   ctx.hufftree->size = 1 << ctx.bitsize;
   ctx.nodesPool = ctx.hufftree->nodesPool_heap = (Node*) calloc(ctx.hufftree->size*2, sizeof(Node));  
   ctx.s = s;
   ctx.hufftree->tree = __unpack(&ctx);
   return ctx.hufftree;
}

