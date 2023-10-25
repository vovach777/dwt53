#pragma once
#include <algorithm>
#include <iostream>
#include <type_traits>
#include <cassert>

#if defined(__has_builtin)
  #if __has_builtin(__builtin_clz)
    #define HAS_BUILTIN_CLZ
  #endif
#else
     #if defined(_MSC_VER)
        inline int __builtin_clz(unsigned long mask)
        {
            unsigned long where;
            // Search from LSB to MSB for first set bit.
            // Returns zero if no set bit is found.
            if (_BitScanReverse(&where, mask))
                return static_cast<int>(31 - where);
            return 32; // Undefined Behavior.
        }
        #define HAS_BUILTIN_CLZ
     #endif
#endif

struct Range {
    int min;
    int max;
};

#ifndef DUMMY
    #define DUMMY
#endif

inline int ilog2_32(uint32_t v, int infinity_val = 1)
{
    if (v == 0)
        return infinity_val;
#ifdef HAS_BUILTIN_CLZ
    return 32 - __builtin_clz(v);
#else
    int count = 0;
    while (v) {
        count++;
        v >>= 1;
    }
    return count;
#endif
}

namespace ffmpeg {

template <typename T>
inline auto FFABS(T a) {
    return ((a) >= 0 ? (a) : (-(a)));
}

template <typename T>
inline auto FFSIGN(T a) {
    return ((a) > 0 ? 1 : -1);
};

template<typename T1, typename T2>
inline auto FFMIN(T1 a, T2 b) {
    return std::min<T1>(a,b);
}

template<typename T1, typename T2>
inline auto FFMAX(T1 a, T2 b) {
    return std::max<T1>(a,b);
}

/*

 * Define AV_[RW]N helper macros to simplify definitions not provided

 * by per-arch headers.

 */

#ifndef _MSVC_LANG
union unaligned_64 { uint64_t l; } __attribute__((packed)) __attribute__((may_alias));
union unaligned_32 { uint32_t l; } __attribute__((packed)) __attribute__((may_alias));
union unaligned_16 { uint16_t l; } __attribute__((packed)) __attribute__((may_alias));
#else
#pragma pack(push, 1)
union unaligned_64 { uint64_t l; };
#pragma pack(pop)

#pragma pack(push, 1)
union unaligned_32 { uint32_t l; };
#pragma pack(pop)

#pragma pack(push, 1)
union unaligned_16 { uint16_t l; };
#pragma pack(pop)
#endif

/**
 * Clear high bits from an unsigned integer starting with specific bit position
 * @param  a value to clip
 * @param  p bit position to clip at
 * @return clipped value
 */
inline unsigned av_mod_uintp2_c(unsigned a, unsigned p)
{
    return a & ((1U << p) - 1);
}


inline int sign_extend(int val, unsigned bits)
{
    unsigned shift = 8 * sizeof(int) - bits;
    union { unsigned u; int s; } v = { (unsigned) val << shift };
    return v.s >> shift;
}
inline int64_t sign_extend64(int64_t val, unsigned bits)
{
    unsigned shift = 8 * sizeof(int64_t) - bits;
    union { uint64_t u; int64_t s; } v = { (uint64_t) val << shift };
    return v.s >> shift;
}
inline  unsigned zero_extend(unsigned val, unsigned bits)
{
    return (val << ((8 * sizeof(int)) - bits)) >> ((8 * sizeof(int)) - bits);
}



constexpr uint16_t av_bswap16(uint16_t x)
{
    x= (x>>8) | (x<<8);
    return x;
}

constexpr uint32_t av_bswap32(uint32_t x)
{
    return ((((x) << 8 & 0xff00) | ((x) >> 8 & 0x00ff)) << 16 | ((((x) >> 16) << 8 & 0xff00) | (((x) >> 16) >> 8 & 0x00ff)));
}

constexpr uint64_t av_bswap64(uint64_t x)
{
    return (uint64_t)av_bswap32(x) << 32 | av_bswap32(x >> 32);
}


 inline int av_log2(uint32_t x) {
    return ilog2_32(x,1)-1;
}
}

template <typename T>
inline T square(const T v)
{
    return v * v;
}

template <typename T>
inline T median(T a, T b, T c)
{
    if (a < b)
    {
        if (b < c)
            return b;
        return (a < c) ? c : a;
    }
    else
    {
        if (b >= c)
            return b;
        return a < c ? a : c;
    }
}

inline int r_shift(int num, int shift)
{

    return (num < 0 ? -(-num >> shift) : num >> shift);
}

template <typename Iterator>
Iterator find_nerest(Iterator begin, Iterator end, typename std::iterator_traits<Iterator>::value_type val)
{
    if (begin == end)
        return end;
    auto it = std::lower_bound(begin, end, val);

    if (it == end)
    {
        return std::prev(end);
    };
    if (it == begin)
        return it;
    if (*it == val)
        return it;

    const auto lower_distance = std::abs(*std::prev(it) - val);
    const auto upper_distance = std::abs(*it - val);
    return lower_distance < upper_distance ? std::prev(it) : it;
}

inline int make_jpair(int bitlen, int value) {
    return bitlen | value << 4;
}
inline int to_jpair(int v)
{
    if (v == 0)
        return 0;
    v = std::clamp(v, -32767, 32767);
    int uv = abs(v);
    int bitlen = ilog2_32(uv,0);
    const int base = 1 << (bitlen-1);
    return make_jpair(bitlen, v < 0 ?  uv - base | base : uv - base);
}

inline int from_jpair( int jpair ) {
    if (jpair == 0)
        return 0;
    int bitlen = jpair & 0xf;
    const int base = 1 << (bitlen-1);
    return (jpair & (base<<4)) ?  -(((jpair >> 4) & base-1) + base) : (((jpair >> 4) & base-1) + base);
}


/* signed <-> unsigned */
inline unsigned s2u(int v)
{
    const int uv = -2 * v - 1;
    return (unsigned)(uv ^ (uv >> 31));
}

inline int u2s(unsigned uv)
{
    const int v = (int)(uv + 1U);
    return v & 1 ? v >> 1 : -(v >> 1);
}

template <typename Container>
typename std::enable_if<std::is_same<typename Container::value_type, int>::value, std::ostream&>::type
operator<<(std::ostream& o, const Container& a) {
    for (auto v : a)
        o << v << " ";
    return o;
}

template <typename Container>
typename std::enable_if<std::is_same<typename Container::value_type, uint8_t>::value, std::ostream&>::type
operator<<(std::ostream& o, const Container& a) {
    for (auto v : a)
        o << static_cast<int>(v) <<  " ";
    return o;
}

template <typename Iterator, typename = std::enable_if_t<std::is_same_v<typename std::iterator_traits<Iterator>::value_type, uint8_t>>>
class Reader {
    // Ограничение на итераторы, указывающие на тип uint8_t:
    // This code is constrained to iterators pointing to elements of type uint8_t.
    // It will fail to compile if the iterator's value type is not uint8_t.
public:
    Reader(Iterator begin, Iterator end) : beginIt(begin), endIt(end) {}

    uint8_t read_u8() {
        if (beginIt == endIt) {
            throw std::range_error("End of data reached");
        }
        auto value =  *beginIt++;
        return value;
    }
    uint16_t read_u16r() {
        return (read_u8() << 8) | read_u8();
    }

    int32_t read_u24r() {

        int32_t val = read_u8()<<16;
            val |= read_u8()<<8;
            val |= read_u8();
        return val;
    }
    template <typename T>
    const T* read() {
        auto res = reinterpret_cast<const T*>(&*beginIt);
        beginIt += sizeof(T);
        return res;
    }

    void seek_to_end() {
        beginIt = endIt;
    }
    bool is_end() {
        return beginIt == endIt;
    }
    inline Iterator begin() {
        return beginIt;
    }
    inline Iterator end() {
        return endIt;
    }
    uint8_t operator *() {
        return (is_end()) ? 0 : *beginIt;
    }
    auto operator++(int) {
        auto res = *this;
        ++beginIt;
        return res;
    }
private:
    Iterator beginIt;
    Iterator endIt;
};



/*
    https://stackoverflow.com/a/49451507/1062758
*/
template<class T>class span {
public:
    inline span() : _data(0), _size(0) {}
    inline span(T* d, size_t s) : _data(d), _size(s) {}
    inline T& operator[](size_t index) { return _data[index]; }
    inline const T& operator[](size_t index) const { return _data[index];}
    inline size_t size() const { return _size; };
    inline T* begin() { return _data; }
    inline const T* begin() const { return _data; }

    inline T* end() { return _data+_size; }
    inline const T* end() const { return _data+_size; }
protected:
    T* _data;
    size_t _size;
};

    inline uint32_t rand_xor() {
        static uint32_t seed = 0x55555555;
        seed ^= (seed << 13);
        seed ^= (seed >> 17);
        seed ^= (seed << 5);
        return seed;
        }

    inline  uint64_t rand_xor64() {
       return  static_cast<uint64_t>(rand_xor()) << 32 | rand_xor();
    }

inline uint64_t ror1_ip(uint64_t &x)
{
   return (x >> 1) | (x << 63);
}

inline int tinyint_to_int(int v)
{
    if (v == 0)
        return 0;
    int sign = v < 0 ? (v=-v,-1) : 1;
    return (1 << (v-1)) * sign;

}

inline int int_to_tinyint(int v)
{
    int sign = v < 0 ? (v=-v,-1) : 1;
    //v >>= 1;
    return ilog2_32(v+(v >> 1),0)*sign;
}


inline int int_to_tinyint(unsigned v)
{
    return ilog2_32(v+(v >> 1),0);
}


/*
   0-4-4
*/
inline int int_to_smallint(int v) {
    if (v < 0x20) {
        //printf("%8x) denorm as is: %d\n",v,v);
        return v;
    }
    int sign = v < 0? (v=-v,-1) : 1;
    int exp = ilog2_32(v,1)-1;
    if (exp-3 >= 15) {
        int fraction = exp - 18;
        assert(fraction<16);
        //printf("%8x) overflow => %02x\n",v, 0xf0 + fraction);
        return (0xf0 + fraction) * sign;
    }
    int fraction = v & (1 << exp)-1;
    //uint32_t fraction_bits = fraction*16/(1<<exp);
    assert(exp>=4);
    int fraction_bits = fraction >> exp-4;
    //printf("%8x) %d(%d) . %d (%d) => %02x\n",v, exp, (1 << exp),  fraction,fraction_bits, (exp-3)<<4 | fraction_bits);
    assert( fraction_bits <= 15 );
    return ((exp-3)<<4 | fraction_bits)*sign;

}


inline int smallint_to_int(int v) {
    if (v < 0x20)
        return v;
    int sign = v < 0? (v=-v,-1) : 1;
    int exp = (v >> 4);
    if (exp == 15) {
        return  (1 << 18 + (v & 0xf))*sign;
    }
    int base = 8 << exp;
    int fraction = (v & 0xf)<<(exp-1);
    return (base | fraction) * sign;
}
