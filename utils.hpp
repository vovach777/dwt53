#pragma once
#include <algorithm>

template <typename T>
T square(const T v) {
   return v*v;
}

template <typename T>
static T median(T a, T b, T c) {
   if (a<b) {
      if (b<c)
         return b;
      return  (a < c) ? c : a;
   } else {
      if (b>=c)
         return b;
      return a < c ? a : c;
   }
}

static int r_shift( int num, int shift) {
    
    return (num < 0 ? -(-num >> shift) : num >> shift);
    
}

template <typename Iterator>
Iterator find_nerest(Iterator begin, Iterator end, typename std::iterator_traits<Iterator>::value_type val)
{
    if (begin == end)
        return end;
    auto it = std::lower_bound(begin, end, val);

    if (it == end ) {
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
