// this is snapshot of project here: https://godbolt.org/z/8f9fMGTKq
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>
#include <chrono>
#include "myargs/myargs.hpp"

#include "dwt53.hpp"
#include "the_matrix.hpp"

#include "utils.hpp"
#include "packmatrix.hpp"
#include "rangecoder.hpp"
#include "dmc.hpp"
#include "cabacH265.hpp"
#include "bitstream.hpp"
#include "bitstream_helper.hpp"
#include "img_load.hpp"

using namespace pack;

template <typename V>
void test_compress_decompress(V && data ) {
    std::cout << "huffman decompression:\t";
    try {
       std::cout << (matrix_is_equal(  huffman::decompress( huffman::compress(data) ), data) ? "Passed!"  :  "FAIL!!!") << std::endl;
    } catch(const std::exception & e) {
        std::cout << "EXCEPTION \"" <<  e.what() << "\"" << std::endl;
    }

    std::cout << "DMC decompression:\t";
    try {
       std::cout << (matrix_is_equal(  DMC::decompress( DMC::compress(data) ), data) ? "Passed!"  :  "FAIL!!!") << std::endl;
    } catch(const std::exception & e) {
        std::cout << "EXCEPTION \"" <<  e.what() << "\"" << std::endl;
    }

    std::cout << "H265 decompression:\t";
    try {
       std::cout << (matrix_is_equal(  CABAC::decompress( CABAC::compress(data) ), data) ? "Passed!"  :  "FAIL!!!") << std::endl;
    } catch(const std::exception & e) {
        std::cout << "EXCEPTION \"" <<  e.what() << "\"" << std::endl;
    }

    std::cout << "range decompression:\t";
    try {
       std::cout << (matrix_is_equal(  rangecoder::decompress( rangecoder::compress(data) ), data) ? "Passed!"  :  "FAIL!!!") << std::endl;
    } catch(const std::exception & e) {
        std::cout << "EXCEPTION \"" <<  e.what() << "\"" << std::endl;
    }

    std::cout << "MTF+DMC decompression:\t";
    try {
       std::cout << "PSNR: " << psnr( data, MTF::decompress( MTF::compress(data) ))  << std::endl;
    } catch(const std::exception & e) {
        std::cout << "EXCEPTION \"" <<  e.what() << "\"" << std::endl;
    }

}

template <typename V, typename S>
void test(V && data, S && description ) {
    std::cout << "** PACK " << std::forward<S>(description) << " **\nteoretic size=\t" << matrix_energy(std::forward<V>(data)) << std::endl;
    std::cout << "huffman size=\t" << huffman::compress(std::forward<V>(data)).size() << std::endl;
    std::cout << "DMC size=\t"     << DMC::compress(std::forward<V>(data)).size()     << std::endl;
    std::cout << "H265 size=\t"    << CABAC::compress(std::forward<V>(data)).size()   << std::endl;
    std::cout << "Range size=\t"   << rangecoder::compress(std::forward<V>(data)).size()   << std::endl;
    std::cout << "MTF+DMC size=\t" << MTF::compress(std::forward<V>(data)).size()   << std::endl;

    std::cout << std::endl;
    test_compress_decompress(std::forward<V>(data));
}


int main(int argc, char**argv) {
    int max_levels = 8;
    dwt2d::Wavelet wavelet = dwt2d::dwt53;
    using myargs::Args;
    Args args;
    args.parse(argc, argv);
    //auto data = make_envelope(32,32,200);
    // cubicBlur3x3(data);
    // cubicBlur3x3(data);
    //auto data = make_gradient(1024,1024,0,128,128,255);
    //auto data = lenna;
     //auto data = make_sky(64,64);
     //auto data = make_random(1024);
     //cubicBlur3x3(data);
     // cubicBlur3x3(data);
     //auto data = make_probability(8,8,0.1,1);
     //cubicBlur3x3(data);
    dwt2d::Transform dwt;
    auto v = load_image_as_yuv420(args[1].data());

    for (auto& data : v )
    {
        // dwt.prepare_transform(max_levels, wavelet, data);
        // auto & transformed = dwt.forward();
        // dwt.quantization(2);
        // test( transformed, "wavelet q=2");
        test( data, "wavelet q=2");
    }



}
