#pragma once

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
