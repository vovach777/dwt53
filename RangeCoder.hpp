#include <cstdint>
#include <vector>
#include <stdexcept>
#include <cassert>
#include "utils.hpp"

namespace pack
{

    class RangeCoderBase
    {
    protected:
        int low = 0;
        int range = 0xFF00;
        uint8_t zero_state[256] = {0};
        uint8_t one_state[256] = {0};
        uint8_t symbol_state[32];
        RangeCoderBase()
        {
            ff_build_rac_states((1LL << 32) / 20, 128 + 64 + 32 + 16);
            std::fill(std::begin(symbol_state), std::end(symbol_state),128);
        }

        void ff_build_rac_states(int factor, int max_p)
        {
            const int64_t one = 1LL << 32;
            int64_t p;
            int last_p8, p8, i;
            // memset(c->zero_state, 0, sizeof(c->zero_state));
            // memset(c->one_state, 0, sizeof(c->one_state));
            last_p8 = 0;
            p = one / 2;
            for (i = 0; i < 128; i++)
            {
                p8 = (256 * p + one / 2) >> 32; // FIXME: try without the one
                if (p8 <= last_p8)
                    p8 = last_p8 + 1;
                if (last_p8 && last_p8 < 256 && p8 <= max_p)
                    one_state[last_p8] = p8;
                p += ((one - p) * factor + one / 2) >> 32;
                last_p8 = p8;
            }
            for (i = 256 - max_p; i <= max_p; i++)
            {
                if (one_state[i])
                    continue;
                p = (i * one + 128) >> 8;
                p += ((one - p) * factor + one / 2) >> 32;
                p8 = (256 * p + one / 2) >> 32; // FIXME: try without the one
                if (p8 <= i)
                    p8 = i + 1;
                if (p8 > max_p)
                    p8 = max_p;
                one_state[i] = p8;
            }
            for (i = 1; i < 255; i++)
                zero_state[i] = 256 - one_state[256 - i];
        }
        public:
          uint8_t defaultState() {
            return 128;
          }
    };

    class RangeCoder_compressor : public RangeCoderBase
    {
    protected:
        std::vector<uint8_t> bytestream;
        int outstanding_count = 0;
        int outstanding_byte = -1;

        inline void renorm_encoder()
        {
            // FIXME: optimize
            while (range < 0x100)
            {
                if (outstanding_byte < 0)
                {
                    outstanding_byte = low >> 8;
                }
                else if (low <= 0xFF00)
                {
                    bytestream.push_back(outstanding_byte);
                    for (; outstanding_count; outstanding_count--)
                        bytestream.push_back(0xFF);
                    outstanding_byte = low >> 8;
                }
                else if (low >= 0x10000)
                {
                    bytestream.push_back(outstanding_byte + 1);
                    for (; outstanding_count; outstanding_count--)
                        bytestream.push_back(0x00);
                    outstanding_byte = (low >> 8) & 0xFF;
                }
                else
                {
                    outstanding_count++;
                }
                low = (low & 0xFF) << 8;
                range <<= 8;
            }
        }

    public:
        RangeCoder_compressor() : RangeCoderBase() {}
        inline size_t get_rac_count()
        {
            auto x = bytestream.size() + outstanding_count;
            if (outstanding_byte >= 0)
                x++;
            return 8 * x - ffmpeg::av_log2(range);
        }

        inline void put(bool bit, uint8_t &state) {
            put_rac(state, bit);
        }

        inline void put_rac(uint8_t &state, int bit)
        {
            int range1 = (range * (state)) >> 8;

          assert(state);
          assert(range1 < range);
          assert(range1 > 0);
            if (!bit)
            {
                range -= range1;
                state = zero_state[state];
            }
            else
            {
                low += range - range1;
                range = range1;
                state = one_state[state];
            }
            assert(range);
            renorm_encoder();
        }

        void put_symbol(int v, int is_signed)
        {
            int i;

            if (v)
            {
                const int a = ffmpeg::FFABS(v);
                const int e = ffmpeg::av_log2(a);
                put_rac(symbol_state[0], 0);
                if (e <= 9)
                {
                    for (i = 0; i < e; i++)
                        put_rac(symbol_state[1 + i], 1); // 1..10
                    put_rac(symbol_state[1 + i], 0);

                    for (i = e - 1; i >= 0; i--)
                        put_rac(symbol_state[22 + i], (a >> i) & 1); // 22..31

                    if (is_signed)
                        put_rac(symbol_state[11 + e], v < 0); // 11..21
                }
                else
                {
                    for (i = 0; i < e; i++)
                        put_rac(symbol_state[1 + ffmpeg::FFMIN(i, 9)], 1); // 1..10
                    put_rac(symbol_state[ 1 + 9], 0);

                    for (i = e - 1; i >= 0; i--)
                        put_rac(symbol_state[22 + ffmpeg::FFMIN(i, 9)], (a >> i) & 1); // 22..31

                    if (is_signed)
                        put_rac(symbol_state[11 + 10], v < 0); // 11..21
                }
            }
            else
            {
                put_rac(symbol_state[0], 1);
            }
        }

        /**

         * Terminates the range coder

         * @param version version 0 requires the decoder to know the data size in bytes

         *                version 1 needs about 1 bit more space but does not need to

         *                          carry the size from encoder to decoder

         */

        /* Return bytes written. */
        std::vector<uint8_t> finish()
        {
            range = 0xFF;
            low += 0xFF;
            renorm_encoder();
            range = 0xFF;
            renorm_encoder();
            std::vector<uint8_t> res;
            res.swap(bytestream);
            return res;
        }
        inline size_t get_bytes_count() {
            return bytestream.size();
        }
        inline  std::vector<uint8_t> get_bytes()
        {
            std::vector<uint8_t> res;
            res.swap(bytestream);
            bytestream.reserve(res.capacity());
            return res;
        }

    };

    template <typename Iterator>
    class RangeCoder_decompressor : public RangeCoderBase
    {
    protected:
        Reader<Iterator> reader;
        int overread = 0;

        inline void refill()
        {
            if (range < 0x100)
            {
                range <<= 8;
                low <<= 8;
                if (reader.is_end())
                    overread++;
                 else
                    low += reader.read_u8();
            }
        }

    public:
        RangeCoder_decompressor(Iterator begin, Iterator end) : RangeCoderBase(), reader(begin, end)
        {
            low = reader.read_u16r();
            if (low >= 0xFF00)
            {
                low = 0xFF00;
                reader.seek_to_end();
            }
        }

        inline int get_rac(uint8_t &state)
        {
            int range1 = (range * (state)) >> 8;
            range -= range1;
            if (low < range)
            {
                state = zero_state[state];
                refill();
                return 0;
            }
            else
            {
                low -= range;
                state = one_state[state];
                range = range1;
                refill();
                return 1;
            }
        }

        int get_symbol(int is_signed)
        {
            if (get_rac(symbol_state[0]))
                return 0;
            else
            {
                int i, e;
                unsigned a;
                e = 0;
                while (get_rac(symbol_state[1 + ffmpeg::FFMIN(e, 9)]))
                { // 1..10
                    e++;
                    if (e > 31)
                        throw std::runtime_error("AVERROR_INVALIDDATA");
                }

                a = 1;
                for (i = e - 1; i >= 0; i--)
                    a += a + get_rac(symbol_state[22 + ffmpeg::FFMIN(i, 9)]); // 22..31

                e = -(is_signed && get_rac(symbol_state[11 + ffmpeg::FFMIN(e, 10)])); // 11..21
                return (a ^ e) - e;
            }
        }
    };
}
