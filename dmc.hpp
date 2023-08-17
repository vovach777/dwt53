#pragma once
/*   Dynamic Markov Compression (DMC)    Version 0.0.0


     Copyright 1993, 1987

     Gordon V. Cormack
     University of Waterloo
     cormack@uwaterloo.ca


     All rights reserved.

     This code and the algorithms herein are the property of Gordon V. Cormack.

     Neither the code nor any algorithm herein may be included in any software,
     device, or process which is sold, exchanged for profit, or for which a
     licence or royalty fee is charged.

     Permission is granted to use this code for educational, research, or
     commercial purposes, provided this notice is included, and provided this
     code is not used as described in the above paragraph.

*/

/*    This program implements DMC as described in

      "Data Compression using Dynamic Markov Modelling",
      by Gordon Cormack and Nigel Horspool
      in Computer Journal 30:6 (December 1987)

      It uses floating point so it isn't fast.  Converting to fixed point
      isn't too difficult.

      comp() and exp() implement Guazzo's version of arithmetic coding.

      pinit(), predict(), and pupdate() are the DMC predictor.

      pflush() reinitializes the DMC table and reclaims space

      preset() starts the DMC predictor at its start state, but doesn't
               reinitialize the tables.  This is used for packetized
               communications, but not here.

*/

#include <cstdint>
#include <vector>

namespace pack {
  

class dmc {
   public:
    dmc() { comp_start();}
    std::vector<uint8_t> encoded;
    dmc(int maxnodes) : maxnodes(maxnodes) { comp_start();}

    void comp_start() {
        encoded.clear();
        max = 0x1000000;
        min = 0;
        pinit();
     }

    auto &get_encoded() { return encoded; }
    void comp(int bit) {      
        int mid = min + predict(max-min-1);
        pupdate(bit != 0);

        if (mid == min) mid++;
        if (mid == (max - 1)) mid--;

        if (bit) {
            min = mid;
        } else {
            max = mid;
        }
        while ((max - min) < 256) {
            if (bit) max--;
            encoded.push_back(static_cast<uint8_t>((min >> 16)&0xff));
            min = (min << 8) & 0xffff00;
            max = ((max << 8) & 0xffff00);
            if (min >= max) max = 0x1000000;
        }
    }

    void encode_eof() {
        min = max - 1; 
        encoded.push_back(static_cast<uint8_t>((min >> 16) & 0xff));
        encoded.push_back(static_cast<uint8_t>((min >> 8) & 0xff));
        encoded.push_back(static_cast<uint8_t>(min & 0xff));
    }

    //-1  = eof, 1 = 1, 0 = 0;
    template <typename Iterator>
    int exp(Iterator &it, Iterator begin, Iterator end) {        
        if (it == begin) {             
            if (std::distance(begin, end) >= 3) {
                max = 0x1000000;
                min = 0;
                val = 0;
                pinit();
                val = *it++ << 16;
                val += *it++ << 8;
                val += *it++;
            } else {
                return -1;
            }
        }
        if (val == (max - 1)) {
            return -1;
        }
        int bit = 0;
        int mid = min + predict(max - min - 1);


        if (mid == min) mid++;
        if (mid == (max - 1)) mid--;
        if (val >= mid) {
            bit = 1;
            min = mid;
        } else {
            bit = 0;
            max = mid;
        }
        pupdate(bit != 0);
        while ((max - min) < 256) {
            if (bit) max--;
            val = (val << 8) & 0xffff00 | (*it++ & 0xff);
            min = (min << 8) & 0xffff00;
            max = ((max << 8) & 0xffff00);
            if (min >= max) max = 0x1000000;
        }
        return bit;
    }

    int get_nodes_count() {
        return nodebuf.size();
    }
    private:
    struct node {
        size_t count[2] = {0,0};
        node *next[2] = {nullptr,nullptr};
        node() = default;
    };

    node *p = nullptr;
    std::vector<std::vector<node>> nodes =
        std::vector<std::vector<node>>(256, std::vector<node>(256));

    std::vector<node> nodebuf;

    void pinit() {
        pflush();
        nodebuf.clear();
        nodebuf.reserve( maxnodes );
    }
    
    void pflush() {
        int i, j;
        for (j = 0; j < 256; j++) {
            for (i = 0; i < 127; i++) {
                nodes[j][i].count[0] = 0.22 * FP_CONST;
                nodes[j][i].count[1] = 0.22 * FP_CONST;
                nodes[j][i].next[0] = &nodes[j][2 * i + 1];
                nodes[j][i].next[1] = &nodes[j][2 * i + 2];
            }
            for (i = 127; i < 255; i++) {
                nodes[j][i].count[0] = 0.22 * FP_CONST;
                nodes[j][i].count[1] = 0.22 * FP_CONST;
                nodes[j][i].next[0] = &nodes[i + 1][0];
                nodes[j][i].next[1] = &nodes[i - 127][0];
            }
        }
        preset();
    }

    void preset() { p = &nodes[0][0]; }

    int predict(int val) {
         return val *  p->count[0] /  (p->count[0] +  p->count[1]); 
    }

    void pupdate(int b) {
        if ((nodebuf.size() < maxnodes) &&  p->count[b] >= threshold &&
            p->next[b]->count[0] + p->next[b]->count[1] >=
                bigthresh + p->count[b]) {       
            node* new_ = &nodebuf.emplace_back();
            auto r_mul = p->count[b];
            auto r_div = p->next[b]->count[1] + p->next[b]->count[0];
            p->next[b]->count[0] -= new_->count[0] = p->next[b]->count[0] * r_mul / r_div;                
            p->next[b]->count[1] -= new_->count[1] = p->next[b]->count[1] * r_mul / r_div;
            new_->next[0] = p->next[b]->next[0];
            new_->next[1] = p->next[b]->next[1];
            p->next[b] = new_;
        }
        p->count[b]+=FP_CONST;
        p = p->next[b];
    }
    static  constexpr int FP_CONST = 256;
    static constexpr  int threshold = 2*FP_CONST;
    static constexpr  int bigthresh = 2*FP_CONST;
    int max = 0x1000000, min = 0, maxnodes = 0x10000, val = 0;
};


}  // namespace pack
