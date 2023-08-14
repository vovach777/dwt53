#pragma once
#include <algorithm>

template <typename T>
inline T square(const T v) {
   return v*v;
}

template <typename T>
inline T median(T a, T b, T c) {
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

inline int r_shift( int num, int shift) {
    
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


    // Function to convert a coefficient into a "category/index" pair in JPEG
    // format
   inline int symbol_to_catindex(int coeff) {
        // If the coefficient is negative, we change its sign to positive
        uint32_t positive = (coeff < 0) ? -coeff : coeff;
        if (positive == 0) return 0;

        // Calculate the category of the coefficient as the number of bits
        // needed to represent its absolute value
        int cat = 32 - __builtin_clz(positive);

        // Calculate the minimum and maximum value in the given category
        int minValue = (1 << (cat - 1));
        int maxValue = (1 << cat) - 1;

        // Calculate the index of the coefficient within the given category
        auto index = (coeff < 0) ? maxValue + coeff
                                 : coeff - minValue + (maxValue - minValue + 1);

        // Return the "category/index" pair in JPEG format
        return cat | index << 4;
    }

    // Function to recover the coefficient from a "category/index" pair in JPEG
    // format
    inline int catindex_to_symbol(int pair) {
        if (pair == 0) return 0;

        // Extract the category and index from the "category/index" pair
        int cat = pair & 0xf;
        int index = pair >> 4;

        // Calculate the minimum and maximum value in the given category
        int minValue = (1 << (cat - 1));
        int maxValue = (1 << cat) - 1;

        // Calculate the index of the positive value within the given category
        auto positive_index = 1 << (cat - 1);

        // Recover the original coefficient
        return (index < positive_index) ? -maxValue + index
                                        : index - positive_index + minValue;
    }

    // Function to recover the coefficient from a "category/index" pair in JPEG
    // format
    inline int catindex_to_symbol(int cat, int index) {
        if (cat == 0) return 0;

        // Calculate the minimum and maximum value in the given category
        int minValue = (1 << (cat - 1));
        int maxValue = (1 << cat) - 1;

        // Calculate the index of the positive value within the given category
        auto positive_index = 1 << (cat - 1);

        // Recover the original coefficient
        return (index < positive_index) ? -maxValue + index
                                        : index - positive_index + minValue;
    }
