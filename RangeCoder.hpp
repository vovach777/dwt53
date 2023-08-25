#pragma once
#include <iostream>


#include <cstdint>
#include <iterator>
#include <vector>
#include <stdexcept>
#include <cassert>

namespace pack {

class RangeBase {
    public:
    using StateType = uint8_t;
    protected:

const std::vector<uint8_t> RENORM_TABLE = {
    6, 5, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

};



template<typename W>
class H265Writer : public H265Base {
public:
    using WriterType = W;

    H265Writer(W& writer)
        : writer(writer), low(0), range(510), buffered_byte(0xff), num_buffered_bytes(0), bits_left(23) {}

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
        low <<= 1;
        if (value) {
            low += range;
        }

        bits_left -= 1;

        if (bits_left < 12) {
            flush_completed();
        }

    }

    void finish() {
        assert(bits_left <= 32);

        if ((low >> (32 - bits_left)) != 0) {
            writer.write_u8((buffered_byte + 1) & 0xff);
            while (num_buffered_bytes > 1) {
                writer.write_u8(0x00);
                num_buffered_bytes -= 1;
            }

            low -= 1 << (32 - bits_left);
        } else {
            if (num_buffered_bytes > 0) {
                writer.write_u8(buffered_byte & 0xff);
            }

            while (num_buffered_bytes > 1) {
                writer.write_u8(0xff);
                num_buffered_bytes -= 1;
            }
        }

        uint32_t bits = 32 - bits_left;

        uint32_t data = low;

        while (bits >= 8) {
            writer.write_u8((data >> (bits - 8)) & 0xff);
            bits -= 8;
        }

        if (bits > 0) {
            writer.write_u8((data << (8 - bits)) & 0xff);
        }
    }

private:

    W& writer;
    uint32_t low;
    uint32_t range;
    uint32_t buffered_byte;
    int32_t num_buffered_bytes;
    int32_t bits_left;

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

            writer.write_u8(byte & 0xff);

            byte = (0xff + carry) & 0xff;
            while (num_buffered_bytes > 1) {
                writer.write_u8(byte & 0xff);
                num_buffered_bytes -= 1;
            }
        } else {
            num_buffered_bytes = 1;
            buffered_byte = lead_byte;
        }
    }
};

template<typename R>
class H265Reader : public H265Base {
public:
    using ReaderType = R;
    H265Reader(R &reader)
        : reader(reader), value(0), range(510), bits_needed(8) {
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

private:
    R &reader;
    uint32_t value;
    uint32_t range;
    int32_t bits_needed;
};


}