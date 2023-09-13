#pragma once
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>
#include <utility>
#include "utils.hpp"

class Codebook {
    public:
    virtual int at(int v) const = 0; //lockup quantized value
    virtual const std::vector<int>& get_codebook() const = 0; //all values
    const int operator[](int v) const { return at(v); }
};


class scalar_adaptive_quantization : public Codebook
{
    public:

    scalar_adaptive_quantization() = default;

    template <typename Iterator, typename init_code_book>
    scalar_adaptive_quantization(Iterator begin, Iterator end, int Q, int n = 256, init_code_book initial_codebook = {-4,-3,-2,-1,0,1,2,3,4} ) : n(n), Q(Q), codebook(initial_codebook)
    {

        enabled = begin != end;
        if (! enabled)
            return;

        const int max_elem = n*2;
        lockup_table.resize(max_elem+1);
        center = lockup_table.data() + n;
        auto& histogram = lockup_table;

        codebook.reserve(n);

        // //preserve details
        // for (int i = -d; i <= d; ++i ) {
        //     codebook.push_back(i);
        // }

        //build histogram
        for (auto it = begin; it != end; ++it)
        {
                histogram[ std::clamp( *it + n,0, max_elem) ] += 1;
        }
        for (auto w : codebook) {
            histogram[ std::clamp( w + n,0, max_elem) ] = 0;
        }
        quantization(histogram.data(), histogram.data()+histogram.size());
        //std::fill( histogram.begin(), histogram.end(), 0);

        std::sort(codebook.begin(),  codebook.end());


        //std::cout << "codebook: " << codebook;

        //lockup table generate
        for (int i=0; i<=max_elem; ++i)
        {
            lockup_table[i] = *find_nerest(codebook.begin(), codebook.end(), i-n);
        }
        //std::cout << "lockup table: " << lockup_table;
    }

    int at(int v) const override {
        if (!enabled)
            return v;
        const int max_elem = static_cast<int>( lockup_table.size()-1 );

        return lockup_table[ std::clamp( v + n,0, max_elem) ];
    }
    const std::vector<int>& get_codebook() const override {
        return codebook;
    }

private:
    void quantization(int* begin, int *end)
    {
        if (begin >= end)
            return;
        auto maxIt = std::max_element(begin, end);

        if ( *maxIt == 0)
            return;

        int value = std::distance( center, maxIt );
        codebook.push_back( value );

        int distance_around_it_max = std::max(1, static_cast<int>( sqrt( 1+abs(value) ) * Q )); //value to quantize step

        auto left_end    = maxIt - distance_around_it_max;
        auto right_begin = maxIt + distance_around_it_max;

        if (left_end    > begin)  quantization(begin, left_end);
        if (right_begin < end)    quantization(right_begin, end);
    }

    std::vector<int> lockup_table;
    std::vector<int> codebook;
    bool enabled = false;
    int *center = nullptr;
    int n = 0;
    int Q = 0;

};
