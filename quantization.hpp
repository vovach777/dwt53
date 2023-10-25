#pragma once
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>
#include <utility>
#include <numeric>
#include "utils.hpp"

class scalar_adaptive_quantization {
    public:
    scalar_adaptive_quantization() = default;

    const int operator[](int v) const { return at(v); }

    scalar_adaptive_quantization(int Q, int n = 255, int details = 4 )
    {
        init(Q,n,details);
    }

    void init(int Q, int n = 255, int details = 4 )
    {
        this->n = n;
        this->Q = Q;
        init_codebook_details(details);
        clear_histogram();
        codebook.reserve(n);
    }

    bool is_enabled() {
        return enabled;
    }
    void init_codebook_details(int details) {
        if (details>=0) {
            codebook.resize(details*2+1);
            std::iota(codebook.begin(), codebook.end(), -details );
            enabled = true;
        } else
            codebook.clear();
    }
    inline void set_histogram(int value, int count) {
           histogram[ std::clamp( value + n,0, n*2) ] = count;
    }
    inline void update_histogram(int value) {
           histogram[ std::clamp( value + n,0, n*2) ] += 1;
    }

    void clear_histogram() {
        histogram.resize(n*2+1);
        std::fill( histogram.begin(), histogram.end(), 0);
        center = histogram.data() + n;
    }
    template <typename Iterator>
    void update_histogram(Iterator begin, Iterator end) {
        //build histogram
        for (auto it = begin; it != end; ++it)
        {
            update_histogram(*it);
        }
    }

    void quantization() {

        if ( histogram.size() == 0 )
            return;

        //remove codebook values from preset codebook values
        int max_elem = histogram.size()-1;
        for (auto w : codebook) {
            histogram[ std::clamp( w + n,0, max_elem ) ] = 0;
        }
        quantization_recursion(histogram.data(), histogram.data()+histogram.size());
        histogram.swap(lockup_table);
        std::sort(codebook.begin(),  codebook.end());

        std::cout << "codebook: " << codebook << std::endl;

        //lockup table generate
        for (int i=0; i<=max_elem; ++i)
        {
            lockup_table[i] = *find_nerest(codebook.begin(), codebook.end(), i-n);
        }
        //std::cout << "lockup table: " << lockup_table;
    }

    int at(int v) const {
        if (lockup_table.size() == 0)
            return v;
        const int max_elem = static_cast<int>( lockup_table.size()-1 );

        return lockup_table[ std::clamp( v + n,0, max_elem) ];
    }
    const std::vector<int>& get_codebook() const {
        return codebook;
    }

private:
    void quantization_recursion(int* begin, int *end)
    {
        if (begin >= end)
            return;
        auto maxIt = std::max_element(begin, end);

        if ( *maxIt == 0)
            return;

        int value = std::distance( center, maxIt );
        codebook.push_back( value );

        int distance_around_it_max =  Q < 0  ? -Q :  std::max<>(1, static_cast<int>( sqrt( 1+abs(value) ) * Q )); //value to quantize step

        auto left_end    = maxIt - distance_around_it_max;
        auto right_begin = maxIt + distance_around_it_max;

        //TODO: remove recursion
        if (left_end    > begin)  quantization_recursion(begin, left_end);
        if (right_begin < end)    quantization_recursion(right_begin, end);
    }

    std::vector<int> lockup_table;
    std::vector<int> codebook;
    std::vector<int> histogram;
    bool enabled = false;
    int *center = nullptr;
    int n = 0;
    int Q = 0;

};
