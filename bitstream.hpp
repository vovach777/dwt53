#pragma once
#include <cstdint>
#include <iostream>
#include <ostream>
#include <vector>
#include <stdexcept>
#include <limits>
#include "utils.hpp"
#include "cached_bitstream.hpp"

class BitWriter {

    public:
    inline void writeBits(int n, uint32_t value ) { put_bits(n, value); }
    inline void writeBit(bool value) { put_bits(1,value);}
    inline void writeBit0() { writeBit(true);}
    inline void writeBit1() { writeBit(false);}
    auto data() const {
        return  pb.buf;
    }
    auto size_in_bits() const { return pb.put_bits_count(); }
    auto size() const {
       return pb.put_bytes_count();
        }

    std::vector<uint8_t> get_current_bytes() {

        std::vector<uint8_t> bytes( data(), data() +  bp.put_bytes_output() );
        pb.bytes = std::vector<uint8_t>(4096,0);
        pb.buf = pb.data();
        buf_end = pb.data() + 4096;
        buf_ptr = buf;
        return bytes;

    }

     std::vector<uint8_t> get_all_bytes() {
        pb.flush_put_bits();
        std::vector<uint8_t> bytes(data(), data() + size());
        return bytes;
     }

    void flush() {
        pb.flush_put_bits(false);
    }

    // Метод для вывода данных (для проверки)
    void printBits( std::ostream& o) const {
        auto count = size_in_bits();
        for (auto it = data(), end=data()+size(); it != end; ++it ) {
            auto byte = *it;
            for (int i = 7; i >= 0; --i) {
                  if (count-- ==  0)
                    return;
                o << ((byte >> i) & 1);
            }
            o << " ";
        }
        //o << "\n";
    }

   private:
      PutBitContext pb;
};

class BitReader {
  public:
    BitReader(const uint8_t *buffer, unsigned int bit_size) : bc(buffer, bit_size) {}
    BitReader(const BitWriter& bw) : BitReader(bw.data(), bw.size_in_bits()) {  }
    BitReader() = default;
    inline bool readBit() {
        return bc.bits_read_bit_be();
    }
    inline auto readBits(unsigned int n) {
        return bc.bits_read_be(n);
    }
    inline ptrdiff_t left() {
        return bc.bits_left_be();
    }
    /**
     * Return buffer size in bits.
     */
    inline size_t bits_size() const
    {
        return bc.size_in_bits;
    }

    inline void check_overread() {
        if ( left() < 0)
            throw std::range_error("out of buffer read!!!");
    }

    // Метод для вывода данных (для проверки)
    void printBits(std::ostream & o) const {
        auto count  = bits_size();
        for (auto it = bc.buffer; it != bc.buffer_end; ++it) {
            auto byte = *it;
            for (int i = 7; i >= 0; --i) {
                if (count-- == 0)
                    return;
                o << ((byte >> i) & 1);
            }
            o << " ";
        }
       // o << "\n";
    }

    private:
    ffmpeg::BitstreamContextBE bc;

};


inline std::ostream& operator<<(std::ostream& o, BitWriter const & a) {
    a.printBits(o);
    return o;
}

inline std::ostream& operator<<(std::ostream& o, BitReader const& a) {
    a.printBits(o);
    return o;
}
