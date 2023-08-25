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
constexpr unsigned int ENTROPY_PRECISION_MAX = 0xFFFF;
constexpr unsigned int ENTROPY_PRECISION_MASK = 0xFFFF;
constexpr unsigned int ENTROPY_HALF_RANGE = 0x7FFF;
constexpr unsigned int ENTROPY_QTR_RANGE = 0x3FFF;
constexpr unsigned int ENTROPY_3QTR_RANGE = 0xBFFD;
constexpr unsigned int ENTROPY_MSB_MASK = 0x8000;
constexpr unsigned int ENTROPY_SMSB_MASK = 0x4000;
constexpr int SYMBOL_EOB = 2;
constexpr int SYMBOL_EOF = 3;

    class DMCBase
    {

// #define ENTROPY_PRECISION_MAX				0xffff
// #define ENTROPY_PRECISION_MASK				0xffff
// #define ENTROPY_HALF_RANGE					0x7fff
// #define ENTROPY_QTR_RANGE					0x3fff
// #define ENTROPY_3QTR_RANGE					0xbffd
// #define ENTROPY_MSB_MASK					0x8000
// #define ENTROPY_SMSB_MASK					0x4000

    struct node;
    protected:
        bool first_flag = true;
        virtual void init()
        {
            reset_model(0x10000);
        }

    public:
        using StateType = node*;
        static StateType defaultState() {
            return StateType{};
        }
        DMCBase() { init(); }
        DMCBase(const uint8_t *buffer, size_t bit_size) : reader(buffer, bit_size) {}
    protected:
        void comp(int bit, bool bypass=false)
        {

            switch (bit) {
                case 0:
                case 1:
                    {
                        auto range = (max - min)-2;
                        int mid = (bypass ? (range) >>1 :  predict(range)) + min;

                        if (!bypass)
                            pupdate(bit != 0);
                        if (bit) {
                            min =  mid + 1;
                        } else {
                            max =  mid;
                        }
                        break;
                    }
                case SYMBOL_EOB: min = max-1;
                        break;
                case SYMBOL_EOF: min = max;
                        break;
                default:
                    throw std::logic_error("can not encode symbol!");
            }

            for (;;)
            {
                if ((max & ENTROPY_MSB_MASK) == (min & ENTROPY_MSB_MASK))
                {
                    /* E1/E2 scaling violation. */
                    bool msb = !!(max & ENTROPY_MSB_MASK);
                    if (msb) {
                        min -= ENTROPY_HALF_RANGE + msb;
                        max -= ENTROPY_HALF_RANGE + msb;
                    }
                    writer.writeBit(msb);
                    flush_inverse_bits(msb);
                }
                else if (max <= ENTROPY_3QTR_RANGE && min > ENTROPY_QTR_RANGE)
                {
                    /* E3 scaling violation. */
                    max -= ENTROPY_QTR_RANGE + 1;
                    min	-= ENTROPY_QTR_RANGE + 1;
                    e3_count += 1;
                }
                else
                {
                    break;
                }
                max = ((max << 0x1) & ENTROPY_PRECISION_MAX) | 0x1;
                min = ((min  << 0x1) & ENTROPY_PRECISION_MAX);
            }
        }


        inline void flush_inverse_bits(bool value)
        {
            value = !value;
            for (auto i = 0; i < e3_count; ++i)
            {
                writer.writeBit(value);
            }
            e3_count = 0;
        }

        int exp(bool bypass = false)
        {
            int res = 4; //unknown value
            if (first_flag)
            {
                max = ENTROPY_PRECISION_MAX;
                min = 0;
                val = 0;
                first_flag = false;

                /* We read in our initial bits with padded tailing zeroes. */
                for (auto i = 0; i < 16; ++i)
                {
                    val <<= 1;
                    val |= reader.readBit();
                }
            }
            if (val == max) {
                res = SYMBOL_EOF;
            } else
            if (val == max-1) {
                res = SYMBOL_EOB;
            } else {
                //0 or 1
                auto range = (max - min)-2;
                int mid = ( bypass ? (range) >>1 :  predict(range)) + min;
                /* Decode our bit. */
                if (val > mid)
                {
                    min = mid + 1;
                    res = 1;
                    if (!bypass)
                        pupdate(true);
                }
                else
                {
                    max = mid;
                    res = 0;
                    if (!bypass)
                        pupdate(false);
                }
            }

            for(;;)
            {
                if (max <= ENTROPY_HALF_RANGE)
                {
                    /* If our high value is less than half we do nothing (but
                    prevent the loop from exiting). */
                }
                else if (min > ENTROPY_HALF_RANGE)
                {
                    max -= (ENTROPY_HALF_RANGE + 1);
                    min	-= (ENTROPY_HALF_RANGE + 1);
                    val -= (ENTROPY_HALF_RANGE + 1);
                }
                else if (max <= ENTROPY_3QTR_RANGE && min > ENTROPY_QTR_RANGE)
                {
                    /* E3 scaling violation. */
                    max -= ENTROPY_QTR_RANGE + 1;
                    min	-= ENTROPY_QTR_RANGE + 1;
                    val -= ENTROPY_QTR_RANGE + 1;
                }
                else
                {
                    break;
                }

                int bit = 0;

                if (reader.left() > 0)
                {
                    bit = reader.readBit();
                }

                max = ((max << 0x1) & ENTROPY_PRECISION_MAX) | 0x1;
                min = ((min << 0x1) & ENTROPY_PRECISION_MAX) | 0x0;
                val = ((val << 0x1) & ENTROPY_PRECISION_MAX) | bit;
            }
            return res;

        }

        public:
        int get_nodes_count()
        {
            return nodebuf.size();
        }

        void preset() { p = &nodes[0][0]; }
        auto get_state() {
            return p;
        }
        void set_state(node* state) {
            p = state;
        }

        void reset_model(int maxnodes)
        {
            for (int j = 0; j < 256; j++)
            {
                for (int i = 0; i < 127; i++)
                {
                    nodes[j][i].count[0] = 0.22 * FP_CONST;
                    nodes[j][i].count[1] = 0.22 * FP_CONST;
                    nodes[j][i].next[0] = &nodes[j][2 * i + 1];
                    nodes[j][i].next[1] = &nodes[j][2 * i + 2];
                }
                for (int i = 127; i < 255; i++)
                {
                    nodes[j][i].count[0] = 0.22 * FP_CONST;
                    nodes[j][i].count[1] = 0.22 * FP_CONST;
                    nodes[j][i].next[0] = &nodes[i + 1][0];
                    nodes[j][i].next[1] = &nodes[i - 127][0];
                }
            }
            preset();
            nodebuf.clear();
            nodebuf.reserve(maxnodes);
        }

        auto finish() {
            comp(SYMBOL_EOF);
            e3_count++;
            auto bit = (min >= ENTROPY_QTR_RANGE);
            writer.writeBit(bit);
            flush_inverse_bits(bit);
            return writer.get_all_bytes();
        }

    private:
        struct node
        {
            size_t count[2] = {0, 0};
            node *next[2] = {nullptr, nullptr};
            node() = default;
        };

        node *p = nullptr;
        std::vector<std::vector<node>> nodes =
            std::vector<std::vector<node>>(256, std::vector<node>(256));

        std::vector<node> nodebuf;

        int predict(int val)
        {
            return val * p->count[0] / (p->count[0] + p->count[1]);
        }

        void pupdate(int b)
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
                    preset();
            }
            p->count[b] += FP_CONST;
            p = p->next[b];
        }

        static constexpr int FP_CONST = 256;
        static constexpr int threshold = 2 * FP_CONST;
        static constexpr int bigthresh = 2 * FP_CONST;
        unsigned int max = ENTROPY_PRECISION_MAX;
        unsigned int min = 0;
        unsigned int val = 0;
        int e3_count = 0;
        BitWriter writer;
        BitReader reader;
    };

    //compressor interface
    class DMC_compressor : public DMCBase {
        public:
        inline void put_bypass(bool b) {
             comp(b, true);
        }
        inline void put(bool b, StateType &state) {
            if ( state != nullptr )
                set_state(state);
             comp(b);
             state = get_state();
        }

    };

    class DMC_decompressor : public DMCBase {
        public:
        explicit DMC_decompressor(const uint8_t *buffer, size_t bit_size) : DMCBase(buffer, bit_size) {}
        inline auto get_bypass() {
            return exp(true);
        }
        inline auto get(StateType &state) {
            if ( state != nullptr )
                set_state(state);
            auto res = exp();
            state = get_state();
            return res;
        }

    };



} // namespace pack
