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
        int maxnodes = 0x100000;
        int threshold = 2;
        int bigthresh = 2;
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

        StateType defaultState()
        {
            return &nodes[0][0];
        }

        DMCModel()
        {
            state = defaultState();
            reset_model(0x10000);
        }

        DMCModel(int maxnodes, int threshold = 2, int bigthresh = 2, bool reset_on_overflow = true)
        {
            state = defaultState();
            reset_model(maxnodes, threshold, bigthresh, reset_on_overflow);
        }

        int get_nodes_count() const
        {
            return nodebuf.size();
        }

        void reset_model(int maxnodes, int threshold = 2, int bigthresh = 2, bool reset_on_overflow = true)
        {
            this->threshold = threshold;
            this->bigthresh = bigthresh;
            this->reset_on_overflow = reset_on_overflow;
            this->maxnodes = maxnodes;
            for (int j = 0; j < 256; j++)
            {
                for (int i = 0; i < 127; i++)
                {
                    nodes[j][i].count[0] = 0.2;
                    nodes[j][i].count[1] = 0.2;
                    nodes[j][i].next[0] = &nodes[j][2 * i + 1];
                    nodes[j][i].next[1] = &nodes[j][2 * i + 2];
                }
                for (int i = 127; i < 255; i++)
                {
                    nodes[j][i].count[0] = 0.2;
                    nodes[j][i].count[1] = 0.2;
                    nodes[j][i].next[0] = &nodes[i + 1][0];
                    nodes[j][i].next[1] = &nodes[i - 127][0];
                }
            }
            nodebuf.clear();
            nodebuf.reserve(maxnodes);
            if (nodebuf.capacity() > maxnodes * 2)
            {
                nodebuf = std::vector<node>();
                nodebuf.reserve(maxnodes);
            }
        }

        inline void reset_model()
        {
            reset_model(nodebuf.capacity(), threshold, bigthresh, reset_on_overflow);
        }

        inline float predict() const
        {
            return state->count[0] / (state->count[0] + state->count[1]);
        }

        void update_model(bool b)
        {
            if (!(!reset_on_overflow && nodebuf.size() == maxnodes))
            {

                auto &count_b = state->count[b];
                auto &count_next_b = state->next[b]->count;
                auto count_next_b_sum = count_next_b[0] + count_next_b[1];
                if (
                    count_b >= threshold &&
                    count_next_b_sum >= bigthresh + count_b)
                {
                    if (nodebuf.size() < maxnodes)
                    {
                        node *new_ = &nodebuf.emplace_back();
                        const float r = count_b / count_next_b_sum;
                        const float new_c0 = new_->count[0] = count_next_b[0] * r;
                        const float new_c1 = new_->count[1] = count_next_b[1] * r;

                        count_next_b[0] -= new_c0;
                        count_next_b[1] -= new_c1;

                        new_->next[0] = state->next[b]->next[0];
                        new_->next[1] = state->next[b]->next[1];
                        state->next[b] = new_;
                    }
                    else
                    {
                        if (reset_on_overflow)
                        {
                            reset_model();
                            state = defaultState();
                        }
                    }
                }
            }
            state->count[b] += 1.0f;
            state = state->next[b];
        }

    private:
        std::vector<std::vector<node>> nodes =
            std::vector<std::vector<node>>(256, std::vector<node>(256));

        std::vector<node> nodebuf;
        StateType state;

        int threshold = 2;
        int bigthresh = 2;
        size_t maxnodes = 0x100000;
        bool reset_on_overflow = true;
    };

    template <int models_nb = 1>
    class DMC_compressor
    {
    public:
        using ModelID = int;
        DMC_compressor() {}
        constexpr DMC_compressor(const DMCModelConfig &config)
        {
            for (int i = 0; i < models_nb; ++i)
                models.emplace_back(config.maxnodes, config.threshold, config.bigthresh, config.reset_on_overflow);
        }

        inline void put(bool bit, ModelID model_id)
        {

            auto &model = models.at(model_id);

            int mid = min + (max - min) * model.predict();
            model.update_model(bit);

            if (mid == min)
                mid++;
            if (mid == max)
                mid--;
            assert(mid >= min);
            assert(mid <= max);
            if (bit)
            {
                min = mid;
            }
            else
            {
                max = mid;
            }
            while ((max - min) < 256)
            {
                bytestream.push_back((min >> 16) & 0xff);
                min = (min << 8) & 0xffff00;
                max = ((max << 8) & 0xffff00);
                if (min >= max)
                    max = 0xffffff;
            }
        }
        template <uint32_t bits>
        void put_symbol_bits(uint32_t value)
        {
            auto mask = 1 << (bits - 1);
            for (int i = 0; i < bits; i++)
            {
                put(value & mask, i % models_nb);
                mask >>= 1;
            }
        }

        template <uint32_t bits>
        void put_symbol_bits(uint32_t value, ModelID model_id)
        {
            auto mask = 1 << (bits - 1);
            while (mask)
            {
                put(value & mask, model_id);
                mask >>= 1;
            }
        }

        void put_symbol(int v, int is_signed)
        {
            int i;

            if (v)
            {
                const int a = ffmpeg::FFABS(v);
                const int e = ffmpeg::av_log2(a);
                put(0, 0);
                if (e <= 9)
                {
                    for (i = 0; i < e; i++)
                        put(1, (1 + i) % models_nb); // 1..10
                    put(0, (1 + i) % models_nb);

                    for (i = e - 1; i >= 0; i--)
                        put((a >> i) & 1, (22 + i) % models_nb); // 22..31

                    if (is_signed)
                        put(v < 0, (11 + e) % models_nb); // 11..21
                }
                else
                {
                    for (i = 0; i < e; i++)
                        put(1, (1 + ffmpeg::FFMIN(i, 9)) % models_nb); // 1..10
                    put(0, (1 + 9) % models_nb);

                    for (i = e - 1; i >= 0; i--)
                        put((a >> i) & 1, (22 + ffmpeg::FFMIN(i, 9)) % models_nb); // 22..31

                    if (is_signed)
                        put(v < 0, (11 + 10) % models_nb); // 11..21
                }
            }
            else
            {
                put(1, 0);
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
            // std::cout << "DMC nodes: " << get_nodes_count() << std::endl;
            bytestream.push_back((min >> 16) & 0xff);
            bytestream.push_back((min >> 8) & 0xff);
            bytestream.push_back(min & 0xff);
            std::vector<uint8_t> res;
            res.swap(bytestream);
            return res;
        }

        size_t get_nodes_count() const
        {
            size_t result = 0;
            for (const auto &model : models)
            {
                result += model.get_nodes_count();
            }
            return result;
        }

    private:
        std::vector<uint8_t> bytestream;
        std::vector<DMCModel> models;
        int max = 0xffffff;
        int min = 0;
    };

    template <int models_nb, typename Iterator>
    class DMC_decompressor
    {
    public:
        using ModelID = int;

        constexpr DMC_decompressor(Iterator begin, Iterator end, const DMCModelConfig &config) : reader(begin, end)
        {
            val = reader.read_u24r();
            for (int i = 0; i < models_nb; ++i)
                models.emplace_back(config.maxnodes, config.threshold, config.bigthresh, config.reset_on_overflow);
        }
        inline bool get(ModelID model_id)
        {
            bool bit;
            auto &model = models.at(model_id);
            int mid = min + (max - min) * model.predict();

            // std::cout << ">" << mid << std::endl;
            if (mid == min)
                mid++;
            if (mid == max)
                mid--;
            assert(mid >= min);
            assert(mid <= max);
            if (val >= mid)
            {
                bit = true;
                min = mid;
            }
            else
            {
                bit = false;
                max = mid;
            }
            model.update_model(bit);
            while ((max - min) < 256)
            {
                // if(bit)max--;
                val = (val << 8) & 0xffff00 | reader.read_u8();
                min = (min << 8) & 0xffff00;
                max = (max << 8) & 0xffff00;
                if (min >= max)
                    max = 0xffffff;
            }
            return bit;
        }

        int get_symbol(int is_signed)
        {
            if (get(0))
                return 0;
            else
            {
                int i, e;
                unsigned a;
                e = 0;
                while (get((1 + ffmpeg::FFMIN(e, 9)) % models_nb))
                { // 1..10
                    e++;
                    if (e > 31)
                        throw std::runtime_error("AVERROR_INVALIDDATA");
                }

                a = 1;
                for (i = e - 1; i >= 0; i--)
                    a += a + get((22 + ffmpeg::FFMIN(i, 9)) % models_nb); // 22..31

                e = -(is_signed && get((11 + ffmpeg::FFMIN(e, 10)) % models_nb)); // 11..21
                return (a ^ e) - e;
            }
        }

        size_t get_nodes_count()
        {
            size_t result = 0;
            for (const auto &model : models)
            {
                result += model.get_nodes_count();
            }
        }

    private:
        Reader<Iterator> reader;
        int max = 0xffffff;
        int min = 0;
        int val = 0;
        std::vector<DMCModel> models;
    };

    class Encoder_compressor
    {
    public:
        Encoder_compressor(const DMCModelConfig &config) : model(config.maxnodes, config.threshold, config.bigthresh, config.reset_on_overflow), x1_(0), x2_(0xffffffff) {}

        template <uint32_t bits>
        void put_symbol_bits(uint32_t value)
        {
            auto mask = 1 << (bits - 1);
            for (int i = 0; i < bits; i++)
            {
                put(value & mask);
                mask >>= 1;
            }
        }

        void put(int bit)
        {
            const uint32_t p = Discretize( 1 - model.predict() );
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

} // namespace pack
