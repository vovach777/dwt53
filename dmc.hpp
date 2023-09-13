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
#include <cassert>
#include "bitstream.hpp"

namespace pack
{
    struct DMCModelConfig
    {
        size_t maxnodes = 0x100000;
        uint8_t threshold = 2;
        uint8_t bigthresh = 2;
        bool reset_on_overflow = true;
    };

    class DMCModel
    {
    public:
        struct node
        {
            float count[2] = {0, 0};
            node *next[2] = {nullptr, nullptr};
            node() = default;
        };

        using StateType = node *;



        DMCModel()
        {
            reset_model(0x10000);
        }

        DMCModel(int maxnodes, int threshold = 2, int bigthresh = 2, bool reset_on_overflow = true)
        {

            reset_model(maxnodes, threshold, bigthresh, reset_on_overflow);
        }

        int get_nodes_count() const
        {
            return nodebuf.size();
        }

        void reset_model(int maxnodes, int threshold = 1, int bigthresh = 8, bool reset_on_overflow = true)
        {
            this->threshold = threshold;
            this->bigthresh = bigthresh;
            this->reset_on_overflow = reset_on_overflow;
            this->maxnodes = maxnodes;

            nodebuf.clear();
            nodebuf.reserve(maxnodes);
            if (nodebuf.capacity() > maxnodes * 2)
            {
                nodebuf = std::vector<node>();
                nodebuf.reserve(maxnodes);
            }
            maxnodes = nodebuf.capacity();
            state = &nodebuf.emplace_back();
            state->count[0] = 1;
            state->count[1] = 1;
            state->next[0] = state;
            state->next[1] = state;
        }

        inline void reset_model()
        {
            reset_model(nodebuf.capacity(), threshold, bigthresh, reset_on_overflow);
        }

        inline float predict() const
        {
            return state->count[1]  / (state->count[0] + state->count[1]);
        }

        void update_model(bool b)
        {
            if (!(!reset_on_overflow && nodebuf.size() == maxnodes)  || true)
            {

                auto count_b = state->count[b];
                auto count_next_b = state->next[b]->count;
                auto count_next_b_sum = count_next_b[0] + count_next_b[1];
                if ( count_b >= threshold &&
                    count_next_b_sum >= bigthresh + count_b)
                {
                    if (nodebuf.size() < maxnodes)
                    {
                        node *new_ = &nodebuf.emplace_back();
                        const auto r = count_b / count_next_b_sum;
                        const auto new_c0 = new_->count[0] =  count_next_b[0] * r;
                        const auto new_c1 = new_->count[1] =  count_next_b[1] * r;


                        state->next[b]->count[0] = ffmpeg::FFMAX( count_next_b[0] - new_c0, 1.0f);
                        state->next[b]->count[1] = ffmpeg::FFMAX( count_next_b[1] - new_c1, 1.0f);

                        assert(state->next[b]->count[0] > 0);
                        assert(state->next[b]->count[0] > 0);
                        assert(new_c0 > 0);
                        assert(new_c1 > 0);


                        new_->next[0] = state->next[b]->next[0];
                        new_->next[1] = state->next[b]->next[1];
                        state->next[b] = new_;
                    }
                    else
                    {
                        if (reset_on_overflow)
                        {
                            reset_model();
                        } else {
                            extend_count++;
                            if (extend_count == (maxnodes>>8)+256) {
                                extend_count = 0;
                                state =  &nodebuf[ rand_xor() % maxnodes ];
                                //std::cout << std::endl;
                            }
                            // std::cout << std::endl << deep << std::endl;
                            // char in;
                            // std::cin >> in;
                            // if (in == 'y') {
                            //     state = nodebuf.data();
                            // }
                        }
                    }
                }
            }
            state->count[b] += 1.0f;
            state = state->next[b];
        }

    private:
        std::vector<node> nodebuf;
        StateType state;

        int threshold = 2;
        int bigthresh = 2;
        size_t maxnodes = 0x10000000;
        bool reset_on_overflow = true;
        int extend_count = 0;
    };


    class DMC_compressor
    {
    public:
        DMC_compressor(const DMCModelConfig &config) : model(config.maxnodes, config.threshold, config.bigthresh, config.reset_on_overflow), x1_(0), x2_(0xffffffff) {}

        template <int bits>
        void put_symbol_bits(uint64_t value)
        {
            auto mask = 1ULL << (bits - 1);
            for (int i = 0; i < bits; i++)
            {
                put(value & mask);
                mask >>= 1;
            }
        }

        void put(int bit)
        {
            const uint32_t p = Discretize( model.predict() );
            const uint32_t xmid = x1_ + ((x2_ - x1_) >> 16) * p +
                                  (((x2_ - x1_) & 0xffff) * p >> 16);
            if (bit)
            {
                x2_ = xmid;
            }
            else
            {
                x1_ = xmid + 1;
            }
            model.update_model(bit);

            while (((x1_ ^ x2_) & 0xff000000) == 0)
            {
                WriteByte(x2_ >> 24);
                x1_ <<= 8;
                x2_ = (x2_ << 8) + 255;
            }
        }

       void put_symbol(int v, int is_signed)
        {
            int i;

            if (v)
            {
                const int a = ffmpeg::FFABS(v);
                const int e = ffmpeg::av_log2(a);
                put(0);
                if (e <= 9)
                {
                    for (i = 0; i < e; i++)
                        put(1); // 1..10
                    put(0);

                    for (i = e - 1; i >= 0; i--)
                        put((a >> i) & 1); // 22..31

                    if (is_signed)
                        put(v < 0); // 11..21
                }
                else
                {
                    for (i = 0; i < e; i++)
                        put(1); // 1..10
                    put(0);

                    for (i = e - 1; i >= 0; i--)
                        put((a >> i) & 1); // 22..31

                    if (is_signed)
                        put(v < 0); // 11..21
                }
            }
            else
            {
                put(1);
            }
        }
        inline size_t get_bytes_count()
        {
            return bytestream.size();
        }
        inline std::vector<uint8_t> get_bytes()
        {
            std::vector<uint8_t> res;
            res.swap(bytestream);
            bytestream.reserve(res.capacity());
            return res;
        }

        std::vector<uint8_t> finish()
        {
            while (((x1_ ^ x2_) & 0xff000000) == 0)
            {
                WriteByte(x2_ >> 24);
                x1_ <<= 8;
                x2_ = (x2_ << 8) + 255;
            }
            WriteByte(x2_ >> 24);
            std::vector<uint8_t> res;
            res.swap(bytestream);
            return res;
        }

        size_t get_nodes_count() const
        {
            return model.get_nodes_count();
        }

        void reset_model() {
            model.reset_model();
        }

    private:
        inline void WriteByte(uint8_t byte)
        {
            bytestream.push_back(byte);
        }

        inline uint32_t Discretize(float p)
        {
            return 1 + 65534 * p;
        }

        uint32_t x1_, x2_;
        DMCModel model;
        std::vector<uint8_t> bytestream;
    };

    template <typename Iterator>
    class DMC_decompressor {
        public:
        DMC_decompressor(Iterator begin, Iterator end, const DMCModelConfig &config) : reader(begin, end), model(config.maxnodes, config.threshold, config.bigthresh, config.reset_on_overflow), x1_(0), x2_(0xffffffff), x_(0)
        {
            for (int i = 0; i < 4; ++i) {
                x_ = (x_ << 8) + ( reader.is_end() ? 0 : reader.read_u8());
            }
        }
        DMC_decompressor(const Reader<Iterator> & reader, const DMCModelConfig &config) : reader(reader), model(config.maxnodes, config.threshold, config.bigthresh, config.reset_on_overflow), x1_(0), x2_(0xffffffff), x_(0)
        {
            for (int i = 0; i < 4; ++i) {
                x_ = (x_ << 8) + ( this->reader.is_end() ? 0 : this->reader.read_u8());
            }
        }

        bool get() {
            const uint32_t p = Discretize(model.predict());
            const uint32_t xmid = x1_ + ((x2_ - x1_) >> 16) * p +
                (((x2_ - x1_) & 0xffff) * p >> 16);
            int bit = 0;
            if (x_ <= xmid) {
                bit = 1;
                x2_ = xmid;
            } else {
                x1_ = xmid + 1;
            }

            model.update_model(bit);

            while (((x1_^x2_) & 0xff000000) == 0) {
                x1_ <<= 8;
                x2_ = (x2_ << 8) + 255;
                x_ = (x_ << 8) + ReadByte();
            }
            return bit;
        }

        template <uint32_t bits>
        uint32_t get_symbol_bits()
        {
            int res = 0;
            for (int i = 0; i < bits; i++)
            {
                res = res + res + get();
            }
            return res;
        }

        int get_symbol(int is_signed)
        {
            if (get())
                return 0;
            else
            {
                int i, e;
                unsigned a;
                e = 0;
                while (get())
                { // 1..10
                    e++;
                    if (e > 31)
                        throw std::runtime_error("AVERROR_INVALIDDATA");
                }

                a = 1;
                for (i = e - 1; i >= 0; i--)
                    a += a + get(); // 22..31

                e = -(is_signed && get()); // 11..21
                return (a ^ e) - e;
            }
        }

        inline auto position() {
            return reader.begin();
        }

        private:
            inline uint8_t ReadByte() {
                return reader.is_end() ? 0 : reader.read_u8();
            }

            inline uint32_t Discretize(float p) {
                return 1 + 65534 * p;
            }

        uint32_t x1_, x2_, x_;
        Reader<Iterator> reader;
        DMCModel model;
    };

} // namespace pack
