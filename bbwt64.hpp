#pragma once
#include <utility>
#include <vector>
#include <cstdint>
#include <algorithm>
#include "utils.hpp"

inline std::pair<uint64_t, uint8_t> bbwt64_encode(uint64_t value) {

    std::vector<uint64_t> v64(64);
    auto val = value;
    for (auto &v : v64) {
        v = val;
        val = ror1_ip(val);
    }
    std::stable_sort(v64.begin(), v64.end());

    uint64_t res = 0;
    int index = 0;
    uint8_t res_index = 255;
    for (auto v : v64) {
        if (v == value) {
            res_index = index;
        }
        res = res + res + (v&1);
        index++;
    }
    return std::make_pair(res, res_index);
}

bool comp_tuples(const std::pair<bool, int>& a, const std::pair<bool, int> &b) {
	return (a.first < b.first);
}

inline uint64_t bbwt64_decode(uint64_t value, int j) {

    std::vector<std::pair<bool,int>> v64;

    auto mask = 1ULL << 63;
    for (int i = 0; i < 64; ++i) {
        v64.emplace_back( value & mask, i );
        mask >>= 1;
    }

    std::stable_sort(v64.begin(), v64.end(), comp_tuples);

    uint64_t res = 0;
    for (int i=0; i < 64; i++ ) {
        res = res + res + v64.at(j).first;
        j = v64.at(j).second;
    }
    return res;

}

uint64_t mtf64_encode(uint64_t value) {
    std::bitset<64> v64{value};
    bool front = false;
    for (int pos=63; pos >= 0; --pos) {
        if ( v64[pos] == front)
            v64[pos] = false;
        else {
            v64[pos] = true;
            front = !front; //move-to-front
        }
    }
    return v64.to_ullong();
}

uint64_t mtf64_decode(uint64_t value) {
    std::bitset<64> v64{value};
    bool front = false;
    for (int pos=63; pos >= 0; --pos) {

        front = v64[pos] =  ( v64[pos] ) ? !front : front;
    }
    return v64.to_ullong();

}


template <typename Iterator>
inline size_t bwt_encode(Iterator begin, Iterator end) {
    using T = decltype(*begin);

    std::vector<std::vector<T>> ror( std::distance(begin,end));

    auto vec = std::vector<T>(begin,end);
    ror.push_back(vec);
    for (int i=1; i< ror.size() ) {
        std::rotate(vec.rbegin(), vec.rbegin() + 1, vec.rend());
        ror.push_back(vec);
    }
    auto it1 = vec.begin();
    std::stable_sort(ror.begin(), ror.end());
    int res_index = std::distance(vec.begin(), it1);

    uint64_t res = 0;
    int index = 0;
    uint8_t res_index = 255;
    for (auto v : ror) {
        *begin++= v.back();
    }
    return res_index;
}
