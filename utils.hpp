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