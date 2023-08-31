// this is snapshot of project here: https://godbolt.org/z/8f9fMGTKq
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>
#include <chrono>

#include "dwt53.hpp"
#include "the_matrix.hpp"

#include "utils.hpp"
//#include "packmatrix.hpp"
#include "rangecoder.hpp"
#include "dmc.hpp"
#include "cabacH265.hpp"
#include "bitstream.hpp"
#include "bitstream_helper.hpp"
using namespace pack;

the_matrix lenna = {
    {158, 153, 168, 146, 95, 105, 110, 124, 129, 130, 132, 133, 132, 131, 131, 131, 129, 128, 129, 113, 128, 160, 157, 155, 148, 196, 166, 109, 125, 122, 131, 93, },
    {156, 159, 168, 140, 95, 104, 107, 123, 128, 130, 131, 131, 130, 127, 126, 131, 132, 127, 129, 118, 115, 157, 162, 157, 151, 157, 208, 125, 116, 133, 90, 44, },
    {161, 164, 164, 140, 93, 104, 107, 122, 126, 127, 128, 129, 128, 129, 132, 128, 123, 128, 128, 120, 115, 148, 157, 156, 157, 140, 186, 187, 113, 89, 44, 50, },
    {166, 140, 163, 142, 88, 101, 108, 121, 125, 128, 130, 130, 140, 159, 175, 178, 157, 121, 121, 117, 113, 142, 151, 151, 146, 141, 139, 219, 116, 36, 54, 45, },
    {147, 105, 170, 140, 83, 99, 105, 116, 122, 124, 118, 126, 133, 146, 174, 189, 198, 182, 122, 103, 112, 149, 140, 140, 146, 142, 148, 142, 56, 48, 48, 56, },
    {101, 101, 171, 138, 85, 100, 105, 116, 122, 115, 114, 122, 136, 142, 166, 188, 199, 213, 206, 99, 99, 158, 128, 96, 152, 143, 147, 55, 44, 48, 63, 131, },
    {86, 105, 167, 139, 85, 99, 104, 118, 119, 110, 118, 134, 137, 135, 173, 195, 202, 201, 214, 193, 96, 152, 139, 45, 116, 157, 84, 44, 54, 47, 121, 163, },
    {88, 103, 167, 139, 81, 99, 101, 126, 123, 107, 126, 130, 132, 166, 184, 188, 194, 201, 200, 220, 159, 141, 132, 62, 171, 210, 64, 49, 44, 96, 160, 159, },
    {96, 108, 169, 144, 83, 102, 100, 138, 119, 106, 121, 128, 151, 171, 176, 178, 185, 192, 190, 199, 201, 165, 156, 192, 215, 216, 77, 38, 64, 152, 159, 155, },
    {96, 109, 173, 148, 85, 101, 95, 146, 135, 102, 125, 141, 145, 159, 171, 173, 173, 182, 186, 177, 179, 192, 201, 206, 196, 177, 51, 45, 123, 164, 157, 156, },
    {93, 110, 173, 148, 85, 101, 91, 151, 158, 113, 131, 135, 141, 148, 145, 150, 157, 150, 155, 178, 191, 197, 208, 194, 156, 117, 32, 82, 158, 157, 157, 156, },
    {98, 115, 174, 149, 83, 101, 89, 151, 167, 117, 127, 131, 136, 116, 85, 104, 76, 71, 166, 197, 194, 200, 165, 131, 133, 61, 42, 125, 165, 158, 155, 154, },
    {101, 112, 172, 150, 83, 98, 94, 121, 178, 124, 124, 127, 99, 63, 59, 68, 82, 168, 188, 181, 185, 112, 132, 147, 63, 42, 68, 155, 166, 160, 157, 154, },
    {102, 105, 172, 152, 82, 97, 102, 109, 168, 122, 124, 86, 57, 71, 55, 67, 152, 184, 168, 188, 201, 95, 113, 64, 48, 46, 101, 160, 163, 163, 160, 155, },
    {99, 104, 171, 153, 86, 101, 107, 115, 133, 126, 80, 52, 54, 62, 73, 158, 181, 169, 175, 196, 225, 135, 74, 74, 53, 54, 141, 161, 154, 152, 153, 153, },
    {98, 103, 172, 155, 86, 102, 104, 128, 110, 96, 63, 58, 55, 55, 145, 183, 133, 112, 161, 193, 189, 110, 71, 85, 45, 82, 158, 160, 159, 152, 150, 142, },
    {96, 95, 173, 156, 88, 94, 111, 154, 77, 69, 73, 55, 43, 102, 198, 98, 72, 108, 125, 181, 97, 58, 70, 92, 40, 113, 161, 152, 155, 154, 143, 138, },
    {94, 86, 173, 161, 92, 111, 113, 85, 68, 81, 88, 39, 59, 168, 139, 135, 144, 162, 132, 180, 139, 81, 71, 94, 49, 140, 156, 151, 151, 138, 158, 182, },
    {78, 77, 172, 160, 91, 122, 84, 81, 70, 54, 94, 49, 125, 123, 110, 164, 179, 163, 133, 185, 164, 97, 69, 90, 68, 157, 152, 149, 137, 159, 200, 199, },
    {72, 69, 169, 157, 98, 105, 81, 96, 63, 53, 83, 85, 129, 57, 122, 146, 164, 151, 118, 170, 163, 81, 61, 95, 99, 163, 150, 144, 143, 192, 203, 207, },
    {73, 83, 171, 159, 94, 95, 74, 90, 93, 57, 77, 129, 63, 68, 124, 139, 155, 152, 131, 170, 142, 57, 60, 99, 133, 157, 149, 140, 160, 205, 207, 209, },
    {62, 92, 175, 160, 99, 94, 63, 79, 94, 102, 151, 76, 45, 75, 121, 138, 138, 135, 155, 173, 98, 51, 63, 99, 153, 151, 148, 137, 170, 212, 209, 212, },
    {52, 91, 176, 162, 103, 83, 64, 106, 89, 150, 117, 43, 57, 63, 98, 133, 138, 121, 138, 136, 56, 56, 64, 105, 161, 151, 149, 134, 181, 219, 210, 214, },
    {43, 97, 173, 161, 89, 72, 62, 102, 113, 117, 57, 52, 52, 57, 64, 96, 133, 152, 168, 93, 37, 67, 73, 109, 158, 152, 153, 131, 189, 220, 216, 189, },
    {52, 75, 167, 165, 93, 59, 65, 93, 108, 114, 74, 56, 50, 52, 59, 100, 128, 146, 163, 155, 86, 54, 80, 111, 131, 122, 126, 122, 196, 219, 137, 62, },
    {131, 79, 154, 175, 85, 51, 73, 75, 84, 119, 102, 58, 52, 57, 69, 115, 137, 140, 161, 196, 210, 136, 55, 117, 149, 131, 112, 113, 210, 158, 69, 92, },
    {121, 113, 151, 177, 73, 45, 67, 55, 72, 110, 115, 70, 50, 60, 85, 113, 140, 143, 158, 172, 194, 228, 91, 110, 153, 140, 129, 172, 215, 107, 99, 102, },
    {98, 116, 149, 175, 66, 46, 70, 50, 57, 106, 114, 78, 54, 75, 104, 118, 141, 139, 151, 167, 188, 221, 161, 121, 154, 131, 139, 221, 176, 98, 102, 96, },
    {82, 123, 147, 176, 72, 48, 59, 52, 64, 87, 88, 60, 48, 102, 104, 132, 142, 137, 146, 160, 182, 207, 204, 122, 128, 141, 137, 200, 145, 86, 97, 91, },
    {91, 137, 146, 175, 72, 45, 59, 50, 57, 98, 73, 38, 67, 99, 103, 143, 142, 138, 140, 153, 172, 197, 219, 122, 81, 68, 154, 186, 82, 86, 96, 79, },
    {108, 146, 151, 171, 61, 49, 56, 68, 56, 89, 64, 39, 80, 98, 130, 138, 142, 144, 140, 146, 162, 184, 217, 159, 94, 86, 144, 101, 74, 85, 100, 69, },
    {73, 164, 159, 162, 51, 58, 65, 79, 75, 72, 63, 69, 113, 128, 131, 135, 140, 145, 146, 147, 158, 175, 206, 182, 92, 123, 114, 96, 83, 100, 89, 59, },
};

// template <typename V, typename S>
// void test(V && data, S && description ) {
//     std::cout << "** PACK " << description << " **\nteoretic size=\t" << matrix_energy(data) << std::endl;
//     std::cout << "huffman size=\t" << huffman::compress(data).size() << std::endl;
//     std::cout << "DMC size=\t"     << DMC::compress(data).size()     << std::endl;
//     std::cout << "H265 size=\t"    << CABAC::compress(data).size()   << std::endl;
//     std::cout << std::endl;

//     test_compress_decompress(data);

// }

// template <typename V>
// void test_compress_decompress(V && data ) {
//     std::cout << "huffman decompression:\t";
//     try {
//        std::cout << (matrix_is_equal(  huffman::decompress( huffman::compress(data) ), data) ? "Passed!"  :  "FAIL!!!") << std::endl;
//     } catch(const std::exception & e) {
//         std::cout << "EXCEPTION \"" <<  e.what() << "\"" << std::endl;
//     }

//     std::cout << "DMC decompression:\t";
//     try {
//        std::cout << (matrix_is_equal(  DMC::decompress( DMC::compress(data) ), data) ? "Passed!"  :  "FAIL!!!") << std::endl;
//     } catch(const std::exception & e) {
//         std::cout << "EXCEPTION \"" <<  e.what() << "\"" << std::endl;
//     }

//     std::cout << "H265 decompression:\t";
//     try {
//        std::cout << (matrix_is_equal(  CABAC::decompress( CABAC::compress(data) ), data) ? "Passed!"  :  "FAIL!!!") << std::endl;
//     } catch(const std::exception & e) {
//         std::cout << "EXCEPTION \"" <<  e.what() << "\"" << std::endl;
//     }

// }



int main() {
    int max_levels = 8;
    dwt2d::Wavelet wavelet = dwt2d::dwt53;
    //auto data = make_envelope(32,32,200);
    // cubicBlur3x3(data);
    // cubicBlur3x3(data);
    //auto data = make_gradient(1024,1024,0,128,128,255);
    //auto data = lenna;
     //auto data = make_sky(64,64);
     auto data = make_random(1024);
     //cubicBlur3x3(data);
     // cubicBlur3x3(data);
     //auto data = make_probability(8,8,0.1,1);
     //cubicBlur3x3(data);

    // dwt2d::Transform codec;
    // codec.prepare_transform(max_levels, wavelet, data);
    // codec.forward();
    // codec.quantization(4);
    // data = codec.get_data();



    H265_compressor enc_cabac;
    auto enc_cabac_state = enc_cabac.defaultState();

    DMC_compressor  enc_dmc;
    auto enc_dmc_state = enc_dmc.defaultState();

    RangeCoder_compressor enc_range;
    auto enc_range_state = enc_range.defaultState();


    auto flatten_data =  flatten(data);
    enc_dmc.reset_model(3000000);

    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;

    std::cout << "Encoding perfomance in ms (lower - better):" << std::endl;

    auto t1 = high_resolution_clock::now();
    for (auto v : flatten_data ) {
        enc_dmc.put_symbol(v,0);
    }
    auto enc_dmc_data = enc_dmc.finish();
    auto t2 = high_resolution_clock::now();
    std::cout << "DMC:\t" << duration_cast<milliseconds>(t2 - t1).count() << std::endl;

    t1 = high_resolution_clock::now();
    for (auto v : flatten_data ) {
        enc_range.put_symbol(v, 0);
    }
    auto enc_range_data = enc_range.finish();
    t2 = high_resolution_clock::now();
    std::cout << "RANGE:\t" << duration_cast<milliseconds>(t2 - t1).count() << std::endl;

    t1 = high_resolution_clock::now();
    for (auto v : flatten_data ) {
        enc_cabac.put_symbol(v,0);
    }
    auto enc_cabac_data = enc_cabac.finish();
    t2 = high_resolution_clock::now();
    std::cout << "CABAC:\t" << duration_cast<milliseconds>(t2 - t1).count() << std::endl;


    std::cout << std::endl;

    auto data_energy = matrix_energy(data);

    std::cout << "Predict\t" << flatten_data.size() << " -> " << data_energy << std::endl;
    std::cout << "CABAC\t"  <<  flatten_data.size() << " -> " << enc_cabac_data.size() << std::endl;
    std::cout << "DMC\t"    << flatten_data.size() << " -> " << enc_dmc_data.size()  << " nodes: " << enc_dmc.get_nodes_count() << std::endl;
    std::cout << "RANGE\t"  << flatten_data.size() << " -> " << enc_range_data.size() << std::endl;

    //decoding...
    H265_decompressor dec_cabac(enc_cabac_data.begin(), enc_cabac_data.end());
    auto dec_cabac_state = dec_cabac.defaultState();

    DMC_decompressor  dec_dmc(enc_dmc_data.begin(), enc_dmc_data.end());
    dec_dmc.reset_model(3000000);
    auto dec_dmc_state = dec_dmc.defaultState();

    auto dec_range = RangeCoder_decompressor(enc_range_data.begin(), enc_range_data.end());
    auto dec_range_state = dec_range.defaultState();

    std::cout << "Decoding..." << std::endl;

    try {

    t1 = high_resolution_clock::now();
    for (auto v : flatten_data ) {
        if ( v != dec_range.get_symbol(0) ) {

            throw std::runtime_error("decode fail!");
        }
    }
    t2 = high_resolution_clock::now();
    std::cout << "RANGE:\t" << duration_cast<milliseconds>(t2 - t1).count() << std::endl;
    } catch (const std::exception & e) {
        std::cerr << "RANGE fail: " << e.what() << std::endl;    }
    try {

    t1 = high_resolution_clock::now();
    for (auto v : flatten_data ) {
        if ( v != dec_cabac.get_symbol(0) ) {

            throw std::runtime_error("decode fail!");
        }
    }
    t2 = high_resolution_clock::now();
    std::cout << "CABAC:\t" << duration_cast<milliseconds>(t2 - t1).count() << std::endl;
    } catch (const std::exception & e) {
        std::cerr << "CABAC fail: " << e.what() << std::endl;
    }


    try {

    t1 = high_resolution_clock::now();
    for (auto v : flatten_data ) {
        if ( v != dec_dmc.get_symbol(0) ) {

            throw std::runtime_error("decode fail!");
        }
    }
    t2 = high_resolution_clock::now();
    std::cout << "DMC:\t" << duration_cast<milliseconds>(t2 - t1).count() << std::endl;
    } catch (const std::exception & e) {
        std::cerr << "DMC fail: " << e.what() << std::endl;
    }



//    test(data,"original");

    // dwt2d::Transform codec;
    // codec.prepare_transform(max_levels, wavelet, data);
    // auto& haar_data = codec.forward();

    // test(haar_data, "wavelet coefficients");

    // codec.quantization(1);

    // test(haar_data, "wavelet coefficients + quantization" );


    // auto haar_data2 = huffman::decompress( huffman::compress(haar_data) );

    // for (int y=0; y < haar_data2.size();y++)
    // for (int x=0; x < haar_data2[y].size();x++) {
    //     if (haar_data2[y][x] != haar_data[y][x]) {
    //           std::cout << "DMC fail!" << std::endl;
    //           abort();
    //     }
    // }
    // std::cout << haar_data;
    // auto& reconstructed = codec.inverse();



    // std::cout << "psnr=" << psnr(data, reconstructed) << std::endl;


    // auto enc = Compressor(DMC_compressor());
    // enc.bit_bypass(1);
    // enc.bit_bypass(0);
    // auto encoded = enc.finish();
    // std::cout << encoded << std::endl;
    // auto dec  = Decompressor(DMC_decompressor(encoded.data(),encoded.size()*8 ));
    // auto bit1 = dec.bit_bypass();
    // auto bit2 = dec.bit_bypass();
    // auto bit3 = dec.bit_bypass();
    // std::cout << bit1 << " " << bit2 << " " << bit3 << std::endl;

    // auto test_bw = BitWriter();
    // test_bw.writeBit(1);
    // test_bw.writeBit(0);
    // test_bw.writeBits(32,0);
    // test_bw.writeBits(32,0);
    // auto vec = test_bw.get_all_bytes();
    // auto test_br = BitReader( vec.data(), vec.size()*8 );
    // auto bit1 = test_br.readBit();
    // auto bit2 = test_br.readBit();
    // std::cout << bit1 << " " << bit2 << std::endl;

}
