#include <iostream>
#include <fstream>
#include <cmath>
#include <utility>
#include <cassert>
#include <algorithm>
#include <execution>
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
#include "dmc.hpp"
#include "bitstream.hpp"
using myargs::Args;
using pack::DMC_compressor;
using pack::DMCModelConfig;
Args args;


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


    DMCModelConfig config;
    config.threshold = 4;
    config.bigthresh = 40;
    config.reset_on_overflow = true;
    config.maxnodes = 1ULL << 23;

    DMC_compressor enc(config);
    int prev_sample = 0;
    bool use_bitplanes = args.has("use-bp");
    for (auto it = sf32, end = sf32 + samples_nb; it < end; ++it )
    {

        int sample = *it * 32768;
        int diff = int_to_tinyint( sample - prev_sample);
        prev_sample = std::clamp( prev_sample + tinyint_to_int(diff), -32768, 32768 );
        if ( !use_bitplanes )
        {
            enc.put_symbol(diff,1);
        } else {
            enc.put( diff < 0 );
            assert( std::abs( diff ) < 16);
            enc.put_symbol_bits<4>( std::abs( diff ) );
        }

    }

    std::string output_filename =  ( args.size() > 2 ) ? args[2] :  args[1] + ".sdmc";
    std::ofstream output(output_filename, std::ios::binary | std::ios::trunc);
    auto bytes = enc.finish();
    output.write(  reinterpret_cast<const char*>( bytes.data() ), bytes.size() );


}