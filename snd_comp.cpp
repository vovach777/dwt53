#include <iostream>
#include <fstream>
#include <cmath>
#include <utility>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#define _chsize_s(d, s) ftruncate(d,s)
#endif

#include "myargs/myargs.hpp"
#include "the_matrix.hpp"
#include "dmc.hpp"
#include "mio.hpp"
#include "utils.hpp"
//#include "quantization.hpp"
#include  "codebook.hpp"
using myargs::Args;
using pack::DMC_compressor;
using pack::DMC_decompressor;
using pack::DMCModelConfig;

namespace G711 {
    constexpr int BIAS = 0x84;		/* Bias for linear code. */
    constexpr int SEG_SHIFT = 4;     /* Left shift for segment number. */
    constexpr int QUANT_MASK = 0xf;  /* Quantization field mask. */
    constexpr int SEG_MASK = 0x70;   /* Segment field mask. */
    constexpr int SIGN_BIT = 0x80;	/* Sign bit for a A-law byte. */
    constexpr int CLIP = 8159;

    int16_t ulaw2linear(uint8_t u_val)
    {
    /* Complement to obtain normal u-law value. */
    u_val = ~u_val;

    /*
        * Extract and bias the quantization bits. Then
        * shift up by the segment number and subtract out the bias.
        */
    int16_t t = ((u_val & QUANT_MASK) << 3) + BIAS;
    t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

    return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
    }


uint8_t linear2ulaw(int16_t pcm_val)
{
   int16_t mask;

   /* Get the sign and the magnitude of the value. */
   pcm_val = pcm_val >> 2;
   if (pcm_val < 0) {
      pcm_val = -pcm_val;
      mask = 0x7F;
   } else {
      mask = 0xFF;
   }
   if ( pcm_val > CLIP ) pcm_val = CLIP;		/* clip the magnitude */
   pcm_val += (BIAS >> 2);

   /* Convert the scaled magnitude to segment number. */
   int seg = 0;
    for (auto val : {0x3F, 0x7F, 0xFF, 0x1FF,0x3FF, 0x7FF, 0xFFF, 0x1FFF}) {
		if (pcm_val <= val)
			 break;
        ++seg;
	}

    if (seg >= 8)
        return static_cast<uint8_t>(0x7F ^ mask);

   /*
   * Combine the sign, segment, quantization bits;
   * and complement the code word.
   */
    return static_cast<uint8_t>( (seg << 4) | ((pcm_val >> (seg + 1)) & 0xF) )  ^ mask;

}
}


static int stepsizeTable2[11] = {
    1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144
};



void createEmptyFile(const std::string& fileName, std::streamsize fileSize) {
    FILE *fp=fopen(fileName.data(), "w");
    _chsize_s(fileno(fp),fileSize);
    fclose(fp);
}


static std::vector<int> amplitude_values{5};


int main(int argc, char**argv) {

    Args args;
    args.parse(argc, argv);

    std::error_code error;

    // if (args.has('c')) {
    //     auto rw_mmap = mio::make_mmap<mio::ummap_sink>(args[1], 0, 0, error);
    //     if (error)
    //     {
    //         std::cout << error.message() << std::endl;
    //         return 1;
    //     }

    //     for (auto& rw : rw_mmap)
    //     {
    //         rw = G711::linear2ulaw(  G711::ulaw2linear(rw) );

    //     }
    //     return 0;

    // }


    // std::ifstream input(args[1],std::ios::binary);
    // if (input.fail())  {
    //     std::cerr << "can not open input file!!!" << std::endl << "ERROR (" << errno << ") : " <<  strerror(errno) << std::endl;
    //     return 1;
    // }

    mio::ummap_source mmap = mio::make_mmap<mio::ummap_source>(args[1], 0, 0, error);
    if (error)
    {
        std::cout << error.message() << std::endl;
        return 1;
    }

    bool trace_opt = args.has("trace");
    bool only_sign_bit = args.has('1');


    if (!args.has('d'))
    {

        // int q_step_opt = args.get("q:s",1,0,10);
        // int q_exp_opt = args.get("q:exp",1,1,128);
        //Codebook codebook(32,q_step_opt, q_exp_opt);
        //Codebook codebook({16});

        DMCModelConfig config;
        config.threshold = 4;
        config.bigthresh = 40;
        config.reset_on_overflow = false;
        config.maxnodes = 1ULL << 23;
        DMC_compressor enc(config);
        int up_scale = args.get('x',1,1,256);

        enc.put_symbol_bits<64>( mmap.size() * up_scale);
        enc.reset_model();
        int valpred = 0;
        int index = 0;
        int step =  stepsizeTable2[index];

        for (auto sample_u8 : mmap) {
            for (int multiple_x = 0; multiple_x < up_scale; ++multiple_x)
            {

                /* G726-16 */
                /* Step 1: compute difference from previous sample, record sign */
                //8 bit alaw -> 8 bit pcm
                int val = r_shift(  G711::ulaw2linear( sample_u8 ), 8);
                int diff = val - valpred;
                bool sign = diff < 0;
                if (sign)
                    diff = -diff;

                enc.put(sign);
                /* Step 2: compare difference to step size, choose magnitude bit */
                /* Step 2.1: Assemble value, update index and step values */

                int vpdiff = step >> 1;
                if (diff > step) {
                    vpdiff += step;
                    index += 2;
                    enc.put(1);
                } else {
                    index -= 1;
                    enc.put(0);
                }
                index = std::clamp(index,0,10);

                step = stepsizeTable2[index];
                    /* Step 3: update previous sample value & clamp */
                if (sign)
                    vpdiff = -vpdiff;

                valpred = std::clamp( valpred + vpdiff, -128, 127 );

                if (trace_opt)
                    std::cout << val << " -> " << valpred << " step:" << step << std::endl;
            }
        }

        auto encoded = enc.finish();

        std::cout << std::endl;
        std::cout << "nodes: " << enc.get_nodes_count() << std::endl;
        std::cout << "compressed size: " << encoded.size() << std::endl;
        std::string output_filename =  ( args.size() > 2 ) ? args[2] :  args[1] + ".sdmc";
        std::ofstream output(output_filename, std::ios::binary);
        if (!output) {
            std::cerr << "can not create output file!!!" << std::endl << "ERROR (" << errno << ") : " <<  strerror(errno) << std::endl;
            return 1;
        }
        output.write(reinterpret_cast<const char*>(encoded.data()),encoded.size());
    } else {

        DMCModelConfig config;
        config.threshold = 4;
        config.bigthresh = 40;
        config.reset_on_overflow = false;
        config.maxnodes = 1ULL << 23;
        DMC_decompressor dec(mmap.cbegin(), mmap.cend(), config);
        // std::vector<char> bytes;
        // bytes.reserve(4096);

        // auto q_step_opt = dec.get_symbol(0);
        // auto q_exp_opt = dec.get_symbol(0);
        //Codebook codebook(32,q_step_opt, q_exp_opt);
        //Codebook codebook({16});
        size_t outsize = dec.get_symbol_bits<64>();
        dec.reset_model();

        std::string output_filename =  ( args.size() > 2 ) ? args[2] :  args[1] + ".ulaw.raw";
        // std::ofstream output(output_filename, std::ios::binary);
        // if (!output) {
        //     std::cerr << "can not create output file!!!" << std::endl << "ERROR (" << errno << ") : " <<  strerror(errno) << std::endl;
        //     return 1;
        // }

        createEmptyFile(output_filename,outsize);
        auto rw_mmap = mio::make_mmap<mio::ummap_sink>(output_filename, 0, outsize, error);
        if (error)
        {
            std::cout << error.message() << std::endl;
            return 1;
        }

        int valpred = 0;
        int index = 0;
        int step =  stepsizeTable2[index];


        if (only_sign_bit) {
            std::cout << "1-bit" << std::endl;
            int prev_sample = 0;
            for (auto & w : rw_mmap)
            {
                int log2 = dec.get_symbol(1);
                bool sign = log2 < 0;
                if (log2 != 0) {
                    int step = 1 << (sign ? -log2 : log2);
                    if (sign)
                        prev_sample -= step;
                    else
                        prev_sample += step;
                }

                w = G711::linear2ulaw(prev_sample);
            }
        }
        else
        {
            std::cout << "2-bit" << std::endl;
            for (auto & w : rw_mmap)
            {
                /* G726-16 */
                /* Step 1: get delta value */
                bool sign = dec.get();
                int  delta = dec.get();
                index =  std::clamp( index + (delta ? 2 : -1), 0, 10);
                int vpdiff = step >> 1;
                if (delta)
                vpdiff += step;
                if (sign)
                vpdiff = -vpdiff;
                /* Step 6 - Update step value */
                step = stepsizeTable2[index];
                valpred = std::clamp( valpred  + vpdiff, -128, 127 );
                w = G711::linear2ulaw(valpred << 8);
            }
        }

    }


}