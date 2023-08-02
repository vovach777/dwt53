//this is snapshot of project here: https://godbolt.org/z/hna7c6eaq
#include "the_matrix.hpp"
#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include "dwt53.hpp"


int main() {

    int details = 1;
    int max_levels = 5;
    int Q = 16;
 
    auto data = make_sin2d(23,17,2,0);
    auto original = data;
    std::cout << "original:" << original;
    compress_data(data, max_levels, Q, details);
    std::cout << "compressed:" << data;
    decompress_data(data, max_levels, Q, details);
    std::cout << "decompressed:" << data;
    std::cout << "psnr=" << psnr(original,data) << std::endl;

     
}