#pragma once

#if defined(__has_builtin) && __has_builtin(__builtin_clz)
    #define HAS_BUILTIN_CLZ
#endif


#include <algorithm>

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

#include <iostream>
#include <type_traits>

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
        return *(beginIt++);
    }

private:
    Iterator beginIt;
    Iterator endIt;
};



