#include <iostream>
#include <fstream>
#include <cmath>
#include <utility>
#include <cassert>
#include <algorithm>
#include <execution>
#include <unordered_map>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#define _chsize_s(d, s) ftruncate(d,s)
#endif
#include "myargs/myargs.hpp"
#include "mio.hpp"
#include "utils.hpp"
//#include "quantization.hpp"
#include "timed.hpp"
#include "huffman.hpp"
#include "bitstream.hpp"
using myargs::Args;
Args args;
using Huffman = pack::Huffman<uint8_t>;

float mix_audio( float signal_1, float signal_2 )
{
    auto sum = signal_1 + signal_2;
    auto sign = sum < 0 ? -1 : 1;
    return sum - (signal_1 * signal_2 * sign);

}

float mix_audio( float * signals, int num)
{
    float result = 0;
    while (num-- > 0) {
        result = mix_audio( result, *signals++ );

    }
    return result;
}

int main(int argc, char** argv)
{

    args.parse(argc, argv);

    std::error_code error;
    mio::ummap_source mmap = mio::make_mmap<mio::ummap_source>(args[1], 0, 0, error);
    if (error)
    {
        std::cout << error.message() << std::endl;
        return 1;
    }
    auto sf32 = reinterpret_cast<const float*>(  mmap.data() );
    auto samples_nb = mmap.size() / 4;

    std::unordered_map<uint8_t, size_t> freq;
    std::vector<int8_t> codes;

    bool stereo_opt = args.has("stereo");

    int prev_sample = 0;
    for (auto it = sf32, end = sf32 + samples_nb; it != end; it += (stereo_opt ? 2 : 1) )
    {

        if (! stereo_opt )
        {
            int sample = *it * 32767;
            int diff = int_to_tinyint( sample - prev_sample);
            prev_sample = std::clamp( prev_sample + tinyint_to_int(diff), -32767, 32767 );
            freq[ std::abs( diff ) ]++;
            codes.push_back( diff );
        } else {
            int a = (it[0] + it[1]) * 16383;
            int b = (it[0] - it[1]) * 32767;
            // int mono = a;
            // int diff = b;
            int diff = int_to_tinyint( a - prev_sample);
            prev_sample = std::clamp( prev_sample + tinyint_to_int(diff), -32767, 32767 );
            freq[ std::abs( diff ) ]++;
            codes.push_back( diff );
            int stereo_diff =  int_to_tinyint(b);
            freq[ std::abs( stereo_diff ) ]++;
            codes.push_back(stereo_diff);
        }

    }
    Huffman huff;
    huff.buildHuffmanTree(freq);
    std::cout << huff.exportDHT();
    BitWriter bit_stream;
    huff.encodeHuffmanTree(bit_stream);
    for (auto code : codes) {
        bit_stream.writeBit( code < 0 );
        huff.encode(bit_stream, std::abs(code));
    }

    std::string output_filename =  ( args.size() > 2 ) ? args[2] :  args[1] + ".shuff";
    std::ofstream output(output_filename, std::ios::binary | std::ios::trunc);
    auto bytes = bit_stream.get_all_bytes();
    output.write(  reinterpret_cast<const char*>( bytes.data() ), bytes.size() );


}