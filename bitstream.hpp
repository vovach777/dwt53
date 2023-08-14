#pragma once
#include <cstdint>
#include <iostream>
#include <ostream>
#include <vector>
#include <stdexcept>

class BitWriter {
/*

static_inline void put_bits(PutBitContext *s, int n, uint32_t value)
{
    check_grow(s);
    uint32_t bit_buf;
    int bit_left;
    bit_left = s->bit_left;
    bit_buf  = s->bit_buf;

    if (n < bit_left) {
        bit_buf     = (bit_buf << n) | value;
        bit_left   -= n;
    } else {
        bit_buf   <<= bit_left;
        bit_buf    |= value >> (n - bit_left);
        *((uint32_t*) s->buf_ptr) = __builtin_bswap32(bit_buf);
        s->buf_ptr += 4;
        bit_left   += 32 - n;
        bit_buf     = value;
    }
    s->bit_buf  = bit_buf;
    s->bit_left = bit_left;
}
*/
    inline void put_bits(int n, uint32_t value) {

    auto bit_left = bit_left_;
    auto bit_buf  = bit_buf_;
    //allways reserve
    if (vec.size() <= size_) {
        vec.resize(size_+1);
    }

    if (n < bit_left) {
        bit_buf     = (bit_buf << n) | value;
        bit_left   -= n;
    } else {
        bit_buf   <<= bit_left;
        bit_buf    |= value >> (n - bit_left);
        vec[size_++] = __builtin_bswap32(bit_buf);
        bit_left   += 32 - n;
        bit_buf     = value;
    }
    bit_buf_  = bit_buf;
    bit_left_ = bit_left;
    }

    public:
    inline void writeBits(int n, uint32_t value ) { put_bits(n, value); }
    inline void writeBit(bool value) { put_bits(1,value);}
    inline void writeBit0() { writeBit(true);}
    inline void writeBit1() { writeBit(false);}
    auto data() const { 
        return  reinterpret_cast<const uint8_t*>( vec.data()); 
    }
    auto data() { 
        return  reinterpret_cast<uint8_t*>( vec.data()); 
    }
    auto size_in_bits() const { return size_*32 + (32-bit_left_); }
    auto size() const {  
       return size_in_bits() + 7 >> 3;
        }
  
    void flush() {
        if (bit_left_ == 32)
            return;
   
        vec[size_] = 0;
        auto byte_by_byte = reinterpret_cast<uint8_t*>( &vec[size_] );     
        auto bit_left = bit_left_;
        auto bit_buf = bit_buf_;
        if (bit_left < 32)
            bit_buf <<= bit_left;
        while (bit_left < 32) {
            *byte_by_byte++ = bit_buf >> (32 - 8);
            bit_buf  <<= 8;
            bit_left  += 8;
        }
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
    std::vector<uint32_t> vec;
    uint32_t bit_buf_ = 0;  // буфер для хранения битов
    int bit_left_ = 32;  // счетчик битов в буфере
    std::vector<uint32_t>::size_type size_; //current vector tail
};

class BitReader {
 

    inline unsigned int get_bits1() {
        size_t _index = index;
        if (index >= size_in_bits) {
            throw std::out_of_range("(index >= size_in_bits)!!!");
        }
        uint8_t result = buffer[_index >> 3];
        result <<= _index & 7;
        result >>= 8 - 1;
        _index++;
        index = _index;
        return result;
    }

    /**
     * Read 1-25 bits.
     */
    inline unsigned int get_bits(int n) {
        // assert(n>0 && n<=25);
        if (n == 0) return 0;
        if (index+n > size_in_bits) {
            throw std::out_of_range("(index+n > size_in_bits)!!!");
        }       
        union unaligned_32 {
            uint32_t l;
        } __attribute__((packed)) __attribute__((may_alias));
        unsigned int tmp;
        unsigned int re_index = index;
        unsigned int re_cache;
        re_cache =
            __builtin_bswap32(
                ((const union unaligned_32 *)(buffer + (re_index >> 3)))->l)
            << (re_index & 7);
        tmp = (((uint32_t)(re_cache)) >> (32 - n));
        re_index += n;
        index = re_index;
        return tmp;
    }

    /**
     * Read 0-32 bits.
     */

    inline unsigned int get_bits_long(int n) {
        // assert(n>=0 && n<=32);
        if (!n) {
            return 0;
        if (n==1)
            return get_bits1();

        } else if (n <= 25) {
            return get_bits(n);
        } else {
            unsigned ret = get_bits(16) << (n - 16);
            return ret | get_bits(n - 16);
        }
    }

  public:
    BitReader(const uint8_t *buffer_, size_t buffer_size, size_t limit_bits=std::numeric_limits<size_t>::max()) : size_in_bits(std::min(buffer_size << 3,limit_bits)) {
        buffer_begin = buffer = buffer_;
        buffer_end = buffer_ + buffer_size;
        index = 0;
    }
    BitReader() = default;
    inline bool readBit() {
        return get_bits1();
    }
    inline auto readBits(int n) {
        return get_bits_long(n);
    }
    // BitReader& operator=(BitReader&& other) {

    // }
    // Метод для вывода данных (для проверки)
    void printBits(std::ostream & o) const {
        auto count  = size_in_bits;
        for (auto it = buffer_begin; it != buffer_end; ++it) {
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

    size_t get_index() { return index; }
    //size_t size_in_bits() { return size_in_bits; }
    size_t bit_left() { return size_in_bits - index; }

   private:
    const uint8_t *buffer_begin;
    const uint8_t *buffer;
    const uint8_t *buffer_end;
    size_t index;
    size_t size_in_bits;
};

inline std::ostream& operator<<(std::ostream& o, BitWriter const & a) {
    a.printBits(o);
    return o;
}

inline std::ostream& operator<<(std::ostream& o, BitReader const& a) {
    a.printBits(o);
    return o;
}
