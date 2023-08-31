#pragma once
#include <iostream>
#include <cstdint>
#include <iterator>
#include <vector>
#include <stdexcept>
#include <cassert>
#include "utils.hpp"

namespace pack {

class H265Base {
    public:
    using StateType = uint8_t;
    StateType defaultState() {
        return 0;
    }
    protected:

const std::vector<uint8_t> NEXT_STATE_MPS = {
    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
    28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
    76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118,
    119, 120, 121, 122, 123, 124, 125, 124, 125, 126, 127
};

const std::vector<uint8_t> NEXT_STATE_LPS = {
    1, 0, 0, 1, 2, 3, 4, 5, 4, 5, 8, 9, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 18, 19, 22,
    23, 22, 23, 24, 25, 26, 27, 26, 27, 30, 31, 30, 31, 32, 33, 32, 33, 36, 37, 36, 37, 38, 39, 38,
    39, 42, 43, 42, 43, 44, 45, 44, 45, 46, 47, 48, 49, 48, 49, 50, 51, 52, 53, 52, 53, 54, 55, 54,
    55, 56, 57, 58, 59, 58, 59, 60, 61, 60, 61, 60, 61, 62, 63, 64, 65, 64, 65, 66, 67, 66, 67, 66,
    67, 68, 69, 68, 69, 70, 71, 70, 71, 70, 71, 72, 73, 72, 73, 72, 73, 74, 75, 74, 75, 74, 75, 76,
    77, 76, 77, 126, 127
};

const std::vector<std::vector<uint8_t>> LPST_TABLE = {
    {128, 176, 208, 240},
    {128, 167, 197, 227},
    {128, 158, 187, 216},
    {123, 150, 178, 205},
    {116, 142, 169, 195},
    {111, 135, 160, 185},
    {105, 128, 152, 175},
    {100, 122, 144, 166},
    {95, 116, 137, 158},
    {90, 110, 130, 150},
    {85, 104, 123, 142},
    {81, 99, 117, 135},
    {77, 94, 111, 128},
    {73, 89, 105, 122},
    {69, 85, 100, 116},
    {66, 80, 95, 110},
    {62, 76, 90, 104},
    {59, 72, 86, 99},
    {56, 69, 81, 94},
    {53, 65, 77, 89},
    {51, 62, 73, 85},
    {48, 59, 69, 80},
    {46, 56, 66, 76},
    {43, 53, 63, 72},
    {41, 50, 59, 69},
    {39, 48, 56, 65},
    {37, 45, 54, 62},
    {35, 43, 51, 59},
    {33, 41, 48, 56},
    {32, 39, 46, 53},
    {30, 37, 43, 50},
    {29, 35, 41, 48},
    {27, 33, 39, 45},
    {26, 31, 37, 43},
    {24, 30, 35, 41},
    {23, 28, 33, 39},
    {22, 27, 32, 37},
    {21, 26, 30, 35},
    {20, 24, 29, 33},
    {19, 23, 27, 31},
    {18, 22, 26, 30},
    {17, 21, 25, 28},
    {16, 20, 23, 27},
    {15, 19, 22, 25},
    {14, 18, 21, 24},
    {14, 17, 20, 23},
    {13, 16, 19, 22},
    {12, 15, 18, 21},
    {12, 14, 17, 20},
    {11, 14, 16, 19},
    {11, 13, 15, 18},
    {10, 12, 15, 17},
    {10, 12, 14, 16},
    {9, 11, 13, 15},
    {9, 11, 12, 14},
    {8, 10, 12, 14},
    {8, 9, 11, 13},
    {7, 9, 11, 12},
    {7, 9, 10, 12},
    {7, 8, 10, 11},
    {6, 8, 9, 11},
    {6, 7, 9, 10},
    {6, 7, 8, 9},
    {2, 2, 2, 2}
};

const std::vector<uint8_t> RENORM_TABLE = {
    6, 5, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};


    inline bool get_mps(uint8_t uc_state) {
    return (uc_state & 1) == 1;
    }

    inline void update_lps(uint8_t &uc_state) {
        uc_state = NEXT_STATE_LPS[static_cast<size_t>(uc_state)];
    }

    inline void update_mps(uint8_t &uc_state) {
        uc_state = NEXT_STATE_MPS[static_cast<size_t>(uc_state)];
    }
    inline auto get_state(uint8_t uc_state) {
        return static_cast<size_t>(uc_state >> 1);
    }

    StateType symbol_state[32] = {0};

};



class H265_compressor : public H265Base {
public:
    explicit H265_compressor()
        :  low(0), range(510), buffered_byte(0xff), num_buffered_bytes(0), bits_left(23) {
        }

    void put_bypass(bool value) {
        low <<= 1;
        if (value) {
            low += range;
        }

        bits_left -= 1;

        if (bits_left < 12) {
            flush_completed();
        }
    }

    void put(bool value, uint8_t & uc_state) {
        uint32_t lps = LPST_TABLE[get_state(uc_state)][((range >> 6) & 3)];

        range -= lps;

        if (value != get_mps(uc_state)) {
            uint32_t num_bits = RENORM_TABLE[lps >> 3];
            low = (low + range) << num_bits;
            range = lps << num_bits;

            update_lps(uc_state);

            bits_left -= num_bits;
        } else {
            update_mps(uc_state);

            if (range >= 256) {
                return;
            }

            low <<= 1;
            range <<= 1;
            bits_left -= 1;
        }

        if (bits_left < 12) {
            flush_completed();
        }
    }

    std::vector<uint8_t> finish() {
        assert(bits_left <= 32);

        if ((low >> (32 - bits_left)) != 0) {
            output_u8.push_back((buffered_byte + 1) & 0xff);
            while (num_buffered_bytes > 1) {
                output_u8.push_back(0x00);
                num_buffered_bytes -= 1;
            }

            low -= 1 << (32 - bits_left);
        } else {
            if (num_buffered_bytes > 0) {
                output_u8.push_back(buffered_byte & 0xff);
            }

            while (num_buffered_bytes > 1) {
                output_u8.push_back(0xff);
                num_buffered_bytes -= 1;
            }
        }

        uint32_t bits = 32 - bits_left;

        uint32_t data = low;

        while (bits >= 8) {
            output_u8.push_back((data >> (bits - 8)) & 0xff);
            bits -= 8;
        }

        if (bits > 0) {
            output_u8.push_back((data << (8 - bits)) & 0xff);
        }
        std::vector<uint8_t> res;
        std::swap(res, output_u8);
        return res;
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



private:
    void flush_completed() {
        uint32_t lead_byte = low >> (24 - bits_left);
        bits_left += 8;
        low &= 0xffffffff >> bits_left;

        if (lead_byte == 0xff) {
            num_buffered_bytes += 1;
        } else if (num_buffered_bytes > 0) {
            uint32_t carry = lead_byte >> 8;
            uint32_t byte = buffered_byte + carry;
            buffered_byte = lead_byte & 0xff;

            output_u8.push_back(byte & 0xff);

            byte = (0xff + carry) & 0xff;
            while (num_buffered_bytes > 1) {
                output_u8.push_back(byte & 0xff);
                num_buffered_bytes -= 1;
            }
        } else {
            num_buffered_bytes = 1;
            buffered_byte = lead_byte;
        }
    }

    std::vector<uint8_t> output_u8;
    uint32_t low;
    uint32_t range;
    uint32_t buffered_byte;
    int32_t num_buffered_bytes;
    int32_t bits_left;
};



template <typename Iterator>
class H265_decompressor : public H265Base {
public:
    H265_decompressor(Iterator begin, Iterator end)
        : reader(begin, end), value(0), range(510), bits_needed(8) {
        value = (static_cast<uint32_t>(reader.read_u8()) << 8) | static_cast<uint32_t>(reader.read_u8());
        bits_needed -= 16;
    }

    bool get_bypass() {
        value <<= 1;
        bits_needed += 1;

        if (bits_needed >= 0) {
            bits_needed = -8;
            value |= static_cast<uint32_t>(reader.read_u8());
        }

        uint32_t scaled_range = range << 7;
        auto result = value - scaled_range;

        if (result > 0xFFFFU) {
            return false;
        } else {
            value = result;
            return true;
        }
    }

    bool get(uint8_t& uc_state) {
        uint32_t local_range = range;
        uint32_t local_value = value;
        uint32_t lps = LPST_TABLE[get_state(uc_state)][((local_range >> 6) & 3)];

        local_range -= lps;

        uint32_t scaled_range = local_range << 7;

        bool bit;

        uint32_t result = local_value - scaled_range;

        if (result > 0xFFFFU) {
            // MPS path

            bit = get_mps(uc_state);
            update_mps(uc_state);

            if (scaled_range < (256 << 7)) {
                local_range = scaled_range >> 6;
                local_value <<= 1;
                bits_needed += 1;

                if (bits_needed == 0) {
                    bits_needed = -8;
                    local_value |= static_cast<uint32_t>(reader.read_u8());
                }
            }
        } else {
            // LPS path

            local_value = result;

            uint32_t num_bits = RENORM_TABLE[(lps >> 3)];
            local_value <<= num_bits;
            local_range = lps << num_bits;

            bit = !get_mps(uc_state);
            update_lps(uc_state);

            bits_needed += num_bits;

            if (bits_needed >= 0) {
                local_value |= static_cast<uint32_t>(reader.read_u8()) << bits_needed;
                bits_needed -= 8;
            }
        }

        range = local_range;
        value = local_value;

        return bit;
    }

        int get_symbol(int is_signed)
        {
            if (get(symbol_state[0]))
                return 0;
            else
            {
                int i, e;
                unsigned a;
                e = 0;
                while (get(symbol_state[1 + ffmpeg::FFMIN(e, 9)]))
                { // 1..10
                    e++;
                    if (e > 31)
                        throw std::runtime_error("AVERROR_INVALIDDATA");
                }

                a = 1;
                for (i = e - 1; i >= 0; i--)
                    a += a + get(symbol_state[22 + ffmpeg::FFMIN(i, 9)]); // 22..31

                e = -(is_signed && get(symbol_state[11 + ffmpeg::FFMIN(e, 10)])); // 11..21
                return (a ^ e) - e;
            }
        }


private:
    Reader<Iterator> reader;
    uint32_t value;
    uint32_t range;
    int32_t bits_needed;
};


}