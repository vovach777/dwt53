#pragma once
#include <algorithm>
#include <iostream>
#include <type_traits>

#if defined(__has_builtin) && __has_builtin(__builtin_clz)
    #define HAS_BUILTIN_CLZ
#endif


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
union unaligned_64 { uint64_t l; } __attribute__((packed)) __attribute__((may_alias));
union unaligned_32 { uint32_t l; } __attribute__((packed)) __attribute__((may_alias));
union unaligned_16 { uint16_t l; } __attribute__((packed)) __attribute__((may_alias));

/**
 * Clear high bits from an unsigned integer starting with specific bit position
 * @param  a value to clip
 * @param  p bit position to clip at
 * @return clipped value
 */
static __attribute__((always_inline))  __attribute__((const)) unsigned av_mod_uintp2_c(unsigned a, unsigned p)
{
    return a & ((1U << p) - 1);
}

inline __attribute__((const)) int sign_extend(int val, unsigned bits)
{
    unsigned shift = 8 * sizeof(int) - bits;
    union { unsigned u; int s; } v = { (unsigned) val << shift };
    return v.s >> shift;
}
inline __attribute__((const)) int64_t sign_extend64(int64_t val, unsigned bits)
{
    unsigned shift = 8 * sizeof(int64_t) - bits;
    union { uint64_t u; int64_t s; } v = { (uint64_t) val << shift };
    return v.s >> shift;
}
inline __attribute__((const)) unsigned zero_extend(unsigned val, unsigned bits)
{
    return (val << ((8 * sizeof(int)) - bits)) >> ((8 * sizeof(int)) - bits);
}



inline __attribute__((always_inline)) __attribute__((const)) uint16_t av_bswap16(uint16_t x)
{
    x= (x>>8) | (x<<8);
    return x;
}

inline __attribute__((always_inline)) __attribute__((const)) uint32_t av_bswap32(uint32_t x)
{
    return ((((x) << 8 & 0xff00) | ((x) >> 8 & 0x00ff)) << 16 | ((((x) >> 16) << 8 & 0xff00) | (((x) >> 16) >> 8 & 0x00ff)));
}

inline uint64_t __attribute__((const)) av_bswap64(uint64_t x)
{
    return (uint64_t)av_bswap32(x) << 32 | av_bswap32(x >> 32);
}

 inline int av_log2(uint32_t x) {
    return (31 - __builtin_clz((x)|1));
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


inline int make_jpair(int bitlen, int value) {
    return bitlen | value << 4;
}
inline int to_jpair(int v)
{
    if (v == 0)
        return 0;
    v = std::clamp(v, -32767, 32767);
    int uv = abs(v);
    int bitlen = 32 - __builtin_clz(uv);
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
