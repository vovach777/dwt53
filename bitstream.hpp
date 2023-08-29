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
    inline void writeBits(int n, uint32_t value ) { pb.put_bits(n, value); }
    inline void writeBits64(int n, uint64_t value ) { pb.put_bits64(n, value); }
    inline void writeBit(bool value) { pb.put_bits(1,value);}
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

        std::vector<uint8_t> bytes;
        std::swap(bytes, pb.bytes);

        pb = ffmpeg::PutBitContext();

        return bytes;

    }

     std::vector<uint8_t> get_all_bytes() {
        pb.flush_put_bits();
        return get_current_bytes();
     }

    void flush() {
        pb.flush_put_bits();
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
      ffmpeg::PutBitContext pb;
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


#if 0
void test() {
    std::vector<uint32_t> test_bits_data;
    BitWriter bw;

    constexpr size_t test_bits_max  = 8000000;

    for (size_t i = 0; i < test_bits_max; i++ ) {

        uint32_t value = rand()*0x55555555 + rand();
        uint32_t bits = ilog2_32(value,1);
        test_bits_data.push_back(bits);
        test_bits_data.push_back(value);
        if (bits == 1)
            bw.writeBit(value);
        else
            bw.writeBits(bits,value);

    }


    auto bin = bw.get_all_bytes();
    BitReader br(bin.data(), bin.size()*8);

    std::cout << br.bits_size() << std::endl;
    bool test_failed = false;
    for (size_t i = 0; i < test_bits_max; i++ ) {
    auto bits = test_bits_data[i*2];
    auto value = test_bits_data[i*2+1];

        if ( bits == 1) {
            if ( br.readBit() != value)
                {
                    std::cerr << "test failed at " << i << std::endl;
                    test_failed = true;
                    break;
                }
        }
        else {
            if (br.readBits(bits) != value)
                {
                    std::cerr << "test failed at " << i << std::endl;
                    test_failed = true;
                    break;
                }
        }

    }

    if (! test_failed) {
        std::cout << "test passed!" << std::endl;
    }
}
#endif
