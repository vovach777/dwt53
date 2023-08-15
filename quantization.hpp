#pragma once
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>
#include <utility>
#include "utils.hpp"


class scalar_adaptive_quantization
{
    public:

    scalar_adaptive_quantization() = default;

    template <typename Iterator>
    scalar_adaptive_quantization(Iterator begin, Iterator end, int Q, int n = 256, int d = 4 ) : center(n), Q(Q)
    {

        enabled = begin != end;
        if (! enabled)
            return;

        const int max_elem = n*2;
        lockup_table.resize(max_elem+1);
        auto& histogram = lockup_table;

        codebook.reserve(256);

        //preserve details
        for (int i = -d; i <= d; ++i ) {
            codebook.push_back(i);
        }

        //build histogram
        for (auto it = begin; it != end; ++it)
        {
            if (abs(*it) > d)
                histogram[ std::clamp( *it + n,0, max_elem) ] += 1;
        }
        quantization(histogram.begin(), histogram.end());
        //std::fill( histogram.begin(), histogram.end(), 0);

        std::sort(codebook.begin(),  codebook.end());


        std::cout << "codebook: " << codebook;

        //lockup table generate
        for (int i=0; i<=max_elem; ++i)
        {
            lockup_table[i] = *find_nerest(codebook.begin(), codebook.end(), i-n);
        }
        //std::cout << "lockup table: " << lockup_table;
    }

    int at(int v) const {
        if (!enabled)
            return v;
        const int max_elem = static_cast<int>( lockup_table.size()-1 );

        return lockup_table[ std::clamp( v + center,0, max_elem) ];
   }
    const int operator[](int v) const { return at(v); }

private:
    void quantization(std::vector<int>::iterator begin, std::vector<int>::iterator end)
    {
        if (begin == end)
            return;
        auto maxIt = std::max_element(begin, end);

        if ( *maxIt == 0)
            return;

        codebook.push_back( std::distance( lockup_table.begin() + center, maxIt ));
        //*maxIt = 0;

        auto clear_left = maxIt == begin ? end : maxIt - 1;
        auto clear_right = maxIt + 1;

        auto distance_around_it_max = sqrt( 1+abs(std::distance(lockup_table.begin() + center, maxIt)) ) * Q;

        while (distance_around_it_max-- > 0) {
            if (clear_left != end) {
               // *clear_left = 0;
                if (clear_left == begin)
                    clear_left = end;
                else
                    --clear_left;
            }
            if (clear_right != end) {
                //*clear_right = 0;
                ++clear_right;
            }
        }
        if (clear_left != end)  quantization(begin, clear_left);
        if (clear_right != end) quantization(clear_right, end);
    }

    std::vector<int> lockup_table;
    std::vector<int> codebook;
    bool enabled = false;
    int center = 0;
    int Q = 0;

};
