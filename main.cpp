//this is snapshot of project here: https://godbolt.org/z/hna7c6eaq
#include "the_matrix.hpp"
#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include "dwt53.hpp"




void test(const int *x, int size, int *L, int *H);
using test_t = decltype(test)*;

class Example {
 
    test_t var = &test;

    public:
    void set_transform() {
  
            var = &test;
     
    }
};
int main() {

    int details = 8;
    int max_levels = 1;
    int Q = 22; //not working for now
 
    auto data = make_envelope(32,32,11);
    //auto data = make_gradient(32,32,0,11,11,22);
    //cubicBlur3x3(data);

    std::cout << "original:" << data;
    auto haar_data = dwt2d::compress_data(data, dwt2d::haar, max_levels); 
    std::cout << "haar values:" << haar_data;
    auto decompressed = dwt2d::decompress_data(haar_data, dwt2d::haar, max_levels);
    std::cout << "haar inv psnr=" << psnr(data, decompressed) << decompressed;
    auto dwt53_data = dwt2d::compress_data(data, dwt2d::dwt53, max_levels); 
    std::cout << "dwt5/3 values:" << dwt53_data;
    decompressed = dwt2d::decompress_data(dwt53_data, dwt2d::dwt53, max_levels);
    std::cout << "dwt5/3 inv psnr=" << psnr(data, decompressed) << decompressed;
     
}