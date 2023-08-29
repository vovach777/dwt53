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
#include "bitstream.hpp"

namespace pack
{
    class DMCBase
    {
    protected:
        struct node;
        bool first_flag = true;
        virtual void init()
        {
            reset_model(0x10000);
        }

    public:
        using StateType = node*;
        StateType defaultState() {
            return &nodes[0][0];
        }
        DMCBase() {
            init();
            std::fill(std::begin(symbol_state), std::end(symbol_state), defaultState() );
        }
        int get_nodes_count()
        {
            return nodebuf.size();
        }


        void reset_model(int maxnodes)
        {
            for (int j = 0; j < 256; j++)
            {
                for (int i = 0; i < 127; i++)
                {
                    nodes[j][i].count[0] = 0.2 * FP_CONST;
                    nodes[j][i].count[1] = 0.2 * FP_CONST;
                    nodes[j][i].next[0] = &nodes[j][2 * i + 1];
                    nodes[j][i].next[1] = &nodes[j][2 * i + 2];
                }
                for (int i = 127; i < 255; i++)
                {
                    nodes[j][i].count[0] = 0.2 * FP_CONST;
                    nodes[j][i].count[1] = 0.2 * FP_CONST;
                    nodes[j][i].next[0] = &nodes[i + 1][0];
                    nodes[j][i].next[1] = &nodes[i - 127][0];
                }
            }
            nodebuf.clear();
            nodebuf.shrink_to_fit();
            nodebuf.reserve(maxnodes);
        }
    protected:

        struct node
        {
            size_t count[2] = {0, 0};
            node *next[2] = {nullptr, nullptr};
            node() = default;
        };

        std::vector<std::vector<node>> nodes =
            std::vector<std::vector<node>>(256, std::vector<node>(256));

        std::vector<node> nodebuf;

        inline int predict(int val, StateType p)
        {
            return val * p->count[0] / (p->count[0] + p->count[1]);
        }

        void pupdate(int b, StateType &p)
        {
            if (p->count[b] >= threshold &&
                p->next[b]->count[0] + p->next[b]->count[1] >=
                    bigthresh + p->count[b])
            {
                if (nodebuf.size() < nodebuf.capacity())
                {
                    node *new_ = &nodebuf.emplace_back();
                    auto r_mul = p->count[b];
                    auto r_div = p->next[b]->count[1] + p->next[b]->count[0];
                    p->next[b]->count[0] -= new_->count[0] = p->next[b]->count[0] * r_mul / r_div;
                    p->next[b]->count[1] -= new_->count[1] = p->next[b]->count[1] * r_mul / r_div;
                    new_->next[0] = p->next[b]->next[0];
                    new_->next[1] = p->next[b]->next[1];
                    p->next[b] = new_;
                } else
                    p = defaultState();
            }
            p->count[b] += FP_CONST;
            p = p->next[b];
        }

        static constexpr int FP_CONST = 256;
        static constexpr int threshold = 2 * FP_CONST;
        static constexpr int bigthresh = 2 * FP_CONST;
        unsigned int max = 0x1000000;
        unsigned int min = 0;
        unsigned int val = 0;
        StateType symbol_state[32];
    };


        class DMC_compressor : public DMCBase {
        public:
        DMC_compressor() : DMCBase() {}
        inline void put(bool bit, StateType &state) {
            if ( state == nullptr )
                state = defaultState();

            auto mid = min + predict((max-min-1), state);
            pupdate(bit, state);
            if (mid == min) mid++;
            if (mid == (max-1)) mid--;

            if (bit) {
                min = mid;
            } else {
                max = mid;
            }
            while ((max-min) < 256) {
                if(bit)max--;
                bytestream.push_back(min >> 16);
                min = (min << 8) & 0xffff00;
                max = ((max << 8) & 0xffff00 ) ;
                if (min >= max) max = 0x1000000;
            }
        }
        void put_symbol(int v, int is_signed)
        {
            int i;

            if (v)
            {
                const int a = ffmpeg::FFABS(v);
                const int e = ffmpeg::av_log2(a);
                put(0, symbol_state[0]);
                if (e <= 9)
                {
                    for (i = 0; i < e; i++)
                        put(1, symbol_state[1 + i]); // 1..10
                    put(0, symbol_state[1 + i]);

                    for (i = e - 1; i >= 0; i--)
                        put((a >> i) & 1, symbol_state[22 + i]); // 22..31

                    if (is_signed)
                        put(v < 0,symbol_state[11 + e]); // 11..21
                }
                else
                {
                    for (i = 0; i < e; i++)
                        put(1,symbol_state[1 + ffmpeg::FFMIN(i, 9)]); // 1..10
                    put(0,symbol_state[ 1 + 9]);

                    for (i = e - 1; i >= 0; i--)
                        put((a >> i) & 1,symbol_state[22 + ffmpeg::FFMIN(i, 9)]); // 22..31

                    if (is_signed)
                        put(v < 0,symbol_state[11 + 10]); // 11..21
                }
            }
            else
            {
                put(1,symbol_state[0]);
            }
        }


        std::vector<uint8_t> finish() {
            //std::cout << "DMC nodes: " << get_nodes_count() << std::endl;
            min = max-1;
            bytestream.push_back( min >> 16 );
            bytestream.push_back((min>>8) & 0xff);
            bytestream.push_back(min & 0xff);
            std::vector<uint8_t> res;
            std::swap(res,bytestream);
            return res;
        }
        private:
            std::vector<uint8_t> bytestream;
    };


    template <typename Iterator>
    class DMC_decompressor : public DMCBase {
        public:

        DMC_decompressor(Iterator begin, Iterator end) : DMCBase(), reader(begin, end)  {
            val = reader.read_u24r();
        }
        inline bool get(StateType &state) {
            if ( state == nullptr )
                state = defaultState();
            auto mid = min + predict( (max-min-1), state);
            if (mid == min) mid++;
            if (mid == (max-1)) mid--;
            if (val == max-1) {
                std::cerr << "end of stream detected by DMC !!!" << std::endl;
            }
            if (val >= mid) {
                bit = 1;
                min = mid;
            } else {
                bit = 0;
                max = mid;
            }
            pupdate(bit != 0, state);
            while ((max-min) < 256) {
                if(bit)max--; //why?
                val = (val << 8) & 0xffff00 | reader.reader_u8();
                min = (min << 8) & 0xffff00;
                max = ((max << 8) & 0xffff00 ) ;
                if (min >= max) max = 0x1000000;
            }
        }


        private:
            Reader<Iterator> reader;
    };




} // namespace pack
