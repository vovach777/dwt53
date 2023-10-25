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
#include "resampler.hpp"
#include "myargs/myargs.hpp"
#include "the_matrix.hpp"
#include "dmc.hpp"
#include "mio.hpp"
#include "utils.hpp"
//#include "quantization.hpp"
#include  "codebook.hpp"
#include "timed.hpp"
#include "bwt_mtf_rle.hpp"
#include "huffman.hpp"
// #define no_dwt2d
// #include "dwt53.hpp"
using myargs::Args;
using pack::DMC_compressor;
using pack::DMC_decompressor;
using pack::DMCModelConfig;
using pack::DMC_compressor_ref_model;
using pack::DMC_decompressor_ref_model;
using pack::DMCModel;



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


inline float linear(float x, float width, float x0, float x1)
{

    return (x1-x0) / width * x + x0;

}

void createEmptyFile(const std::string& fileName, std::streamsize fileSize) {
    FILE *fp=fopen(fileName.data(), "w");
    _chsize_s(fileno(fp),fileSize);
    fclose(fp);
}



struct ADPCM4_codec {
    int prev_sample = 0;
    int step_index = 0;

/* ff_adpcm_step_table[] and ff_adpcm_index_table[] are from the ADPCM
   reference source */
const int8_t ff_adpcm_index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

/**
 * This is the step table. Note that many programs use slight deviations from
 * this table, but such deviations are negligible:
 */
const int16_t ff_adpcm_step_table[89] = {
        7,     8,     9,    10,    11,    12,    13,    14,    16,    17,
       19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
       50,    55,    60,    66,    73,    80,    88,    97,   107,   118,
      130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
      337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
      876,   963,  1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
     2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
     5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};


 int encoder(int sample)
{
    int delta  = sample - prev_sample;
    int diff, step = ff_adpcm_step_table[step_index];
    int nibble = 8*(delta < 0);

    delta= abs(delta);
    diff = delta + (step >> 3);

    if (delta >= step) {
        nibble |= 4;
        delta  -= step;
    }
    step >>= 1;
    if (delta >= step) {
        nibble |= 2;
        delta  -= step;
    }
    step >>= 1;
    if (delta >= step) {
        nibble |= 1;
        delta  -= step;
    }
    diff -= delta;

    if (nibble & 8)
        prev_sample -= diff;
    else
        prev_sample += diff;

    prev_sample = std::clamp( prev_sample, -32768, 32768 );
    step_index  = std::clamp( step_index + ff_adpcm_index_table[nibble], 0, 88);

    return nibble;
}

int decoder(int nibble)
{
    int sign, delta, diff, step;

    step = ff_adpcm_step_table[step_index];
    step_index = std::clamp(step_index + ff_adpcm_index_table[(unsigned)nibble],0,88);

    sign = nibble & 8;
    delta = nibble & 7;
    /* perform direct multiplication instead of series of jumps proposed by
     * the reference ADPCM implementation since modern CPUs can do the mults
     * quickly enough */
    diff = step >> 3;
    if (nibble & 4) diff += step;
    if (nibble & 2) diff += step >> 1;
    if (nibble & 1) diff += step >> 2;

    int predictor = prev_sample;
    if (sign) predictor -= diff;
    else predictor += diff;

    prev_sample = std::clamp(predictor,-32768, 32767);

    return prev_sample;

}

};

#if 0
int tinyint_to_int(int v)
{
    if (v >= -16 && v <= 16)
        return v;

    int  sign = v < 0 ? (v=-v,-1) : 1;

    int  mantissa = v & 0x7;
    int  exponent = (v >> 3) & 0xf;

    int  integer = (4 << (exponent)) + (mantissa << (exponent-1));
    return sign*integer;
}

int int_to_tinyint(int v)
{
    if (v >= -16 && v <= 16)
        return v;

    int sign = v < 0 ? (v=-v,-1) : 1;
    int exponent = ilog2_32(v,0)-3;
    //return (4 << exponent) + ( ( (v >> (exponent-1))  & 0x7) <<(exponent-1));
    return sign*(  (exponent << 3) | ( (v >> (exponent-1)) & 7 ) );
}
#endif

static const float phi = (1 + std::sqrt(5)) / 2; // Золотое сечение
static const float sqrt5 = std::sqrt(5);
static const float log_phi = std::log(phi);

// Функция, которая находит индекс числа Фибоначчи для заданного числа
inline int findFibonacciIndex(int num) {

    if (num == 0)
        return 0;
    int sign = num < 0 ? (num=-num,-1) : 1;

    // Используем формулу Бине, чтобы найти приближенный индекс числа Фибоначчи
    int index = std::round(std::log(num * sqrt5) / log_phi);

    return index*sign;
}

// Функция, которая возвращает число Фибоначчи по его индексу
inline int getFibonacciNumber(int index) {

    if (index == 0)
        return 0;
    int sign = index < 0 ? (index=-index,-1) : 1;
    // Используем формулу Бине, чтобы найти значение числа Фибоначчи
    int fibonacci = std::roundf((std::pow(phi, index) - std::pow(1 - phi, index)) / sqrt5);

    return fibonacci*sign;
}

struct DPCMQ_codec {
    int prev_sample = 0;

int encoder(float samplef)
{
    int sample = samplef * 0x1000000;
    int32_t delta = int_to_tinyint(sample - prev_sample);
    prev_sample = std::clamp( prev_sample + tinyint_to_int(delta), -0xffffff, 0x1000000 );
    return delta;
}

float decoder(int delta)
{

    prev_sample =std::clamp(  prev_sample + tinyint_to_int(delta), -0xffffff, 0x1000000 );
    return prev_sample / static_cast<float>(0x1000000);
}

};

struct DPCMQ_codec_int {
    int prev_sample = 0;
    int clip_min   = -32768;
    int clip_max   = 32767;
    pack::Huffman<uint8_t> huff;
    std::unordered_map<uint8_t, size_t> freq;
   // int center;

    // int clip_diff;
    // int shift_diff;
    // int diff_bits;
    // unsigned int probe = 0;
    // std::vector<uint32_t> codebook;
    // std::vector<int> map_to;
    // std::vector<int> map_from;

    DPCMQ_codec_int(int bits)
    {
        clip_min = -(1 << (bits-1));
        clip_max = (1 << (bits-1))-1;
        // clip_diff  = (1 << (bits-5))-1;
        // shift_diff =  (bits-5-bits_nb );
        // if (  shift_diff < 0)
        // {
        //     diff_bits = bits_nb+shift_diff;
        //     shift_diff = 0;

        // } else
        //   diff_bits = bits_nb;
        // center = (bits-6);
        // int i=0;
        // map_to.resize(32);
        // map_from.resize(32);
        // for (auto v : {0,-1,-2,1,2,-3,-4,-5,-6,3,-7,-8,-9,-10,4})
        // {
        //     int value = center + v;
        //     if (value < 0)
        //       continue;
        //     if (value >= bits)
        //       continue;
        //     map_to[i] = value;
        //     map_from[ value ] = i;
        //     i++;
        //     if ( i == bits-1)
        //         break;
        // }

        // for (int v=-11; i < bits-1; --v)
        // {
        //     int value = center + v;
        //     if (value < 0)
        //        break;
        //     map_to[i] = value;
        //     map_from[ value ] = i;
        //     i++;
        //     if ( i == bits)
        //         break;
        // }
        // assert( i == bits-1);
        // for (; i < 32; ++i)
        // {
        //     map_to[i]   = i;
        //     map_from[i] = i;
        // }
        // for (int i = 0; i < bits; ++i)
        // {
        //     std::cout << i << " " << map_to[i] << " " << map_from[ map_to[i] ] << std::endl;
        // }
        // std::cout << std::endl;

    }

int encoder(int sample)
{

    int diff = int_to_tinyint( sample - prev_sample);

    prev_sample = std::clamp( prev_sample + tinyint_to_int(diff), clip_min, clip_max );
    std::cerr << (char)diff;
    return diff;

    // int round_delta = q_step / (q_step,  delta + (delta >> 1);
    // if (delta > clip_diff )
    //     delta = clip_diff;
    // delta >>= shift_diff;
    //delta = int_to_tinyint(diff);

    //diff_bits = std::max<int>( (bits-5), 4);
    // if ( delta >= codebook.size() )
    // {
    //     codebook.resize( delta + 1, 0);
    // }
    // codebook[ delta ] += 1;




    // if (delta >= map_from.size() ) {
    //     std::cerr << "delta range error: " << delta << " of " << map_from.size()-1 << std::endl;
    //     return 0;
    // }

//     if (probe) {
//         probe--;
//         int diff = sample - prev_sample;
//         int sign = diff < 0 ? -1 : 1;
//         int delta = diff*sign;
//         int index = int_to_tinyint(delta);
//         if (index >= codebook.size()) {
//             codebook.resize(index+1);
//         }
//         float & cw = codebook[ index ];
//         cw = cw ? (cw + delta) / 2.0f : delta;
//         assert(sample == prev_sample + diff);
//         prev_sample = sample;
//         if (probe == 0) {
//             std::cout << "codebook: ";
//             for (const auto v : codebook) {
//                 std::cout << std::fixed << std::setprecision(2) << v << " ";
//             }
//             std::cout << std::endl;
//             for (int i=0; i<codebook.size(); ++i)
//                 mtf.push_back(i);
//         }
//         return diff;
//     } else {
//         int diff = int_to_tinyint(sample - prev_sample);
// //        int sign  = diff < 0 ? -1 : 1;
// //        int qdiff = static_cast<int>(codebook[ std::min<int>( codebook.size()-1, diff*sign) ]) * sign;
//         prev_sample = std::clamp<int>( prev_sample + tinyint_to_int(diff), clip_min, clip_max );
//         return diff;
//         // int res;
//         // mtf_encode(mtf.begin(), mtf.end(), &delta, (&delta)+1, &res );
//         // return res*sign;
//     }
}

void show() {
    // std::cout << "****" << std::endl;
    // uint32_t max_element = *std::max_element(codebook.begin(), codebook.end());
    // for (int i = 0; i < codebook.size(); ++i )
    // {
    //     std::cout << "#" << i << " " << tinyint_to_int(i) << " -> "  << (codebook[i] * 100 / max_element) << ((i==center) ? "*" : "") << std::endl;
    // }
}


int decoder(int diff)
{

    // int sign = diff & 1;
    // int delta = diff >> 1;
    // diff = map_to.at(delta) * sign;
    prev_sample = std::clamp(  prev_sample + tinyint_to_int( diff ), clip_min, clip_max );
    return prev_sample;

    // int sign  = diff < 0 ? -1 : 1;
    // int delta = diff * sign;
    // if (probe) {
    //     probe--;
    //     int index = int_to_tinyint(delta);
    //     if (index > codebook.size()) {
    //         codebook.resize(index+1);
    //     }
    //     float& cw = codebook[ index ];
    //     cw = cw ? ( cw + delta ) / 2.0f : delta;
    //     prev_sample += delta;
    //     if (probe == 0) {
    //         std::cout << "codebook: ";
    //         for (const auto v : codebook) {
    //             std::cout << std::fixed << std::setprecision(2) << v << " ";
    //         }
    //         std::cout << std::endl;
    //     }
    // } else {
    // // if (delta >= codebook.size()) {
    // //     std::cerr << "error in stream!" << std::endl;
    // //     return prev_sample;
    // // }

    //     //prev_sample += std::clamp<int>( prev_sample + static_cast<int>(codebook.at(delta))*sign,  clip_min, clip_max );
    //     prev_sample += std::clamp<int>( prev_sample + tinyint_to_int(diff),  clip_min, clip_max );
    // }
    // return prev_sample;

}

};



// class vector_coding
// {

//     public:


//     void encode(float sample)
//     {
//         int isample = int( round(sample * range) );

//         if  (isample == 0)
//         {
//            sine.push_back(0.0f);
//            return;
//         }

//         if ( (sign ^ isample) >= 0  )     {
//             sine.push_back(sample);
//         } else {
//             cross_ziro_count++;
//             sign = -sign;
//             if ( cross_ziro_count == 2)
//             {
//                 encode();
//             }
//             sine.push_back(sample);
//         }
//     }
//     size_t size() {
//         return codec.size();
//     }
//     void flush() {
//         if (sine.size() > 0) {
//             encode();
//             sign = 1;
//             prev_sine.clear();
//             sine.clear();
//         }
//     }
//     std::vector<std::vector<int>> get_codes() {
//         std::vector<std::vector<int>> ret;
//         ret.swap(codec);
//         return ret;
//     }
//     private:
//     void encode() {
//         cross_ziro_count = 0;

//         prev_sine = resample(prev_sine, sine.size());

//         // for (int i = 0; i < sine.size(); ++i)
//         // {
//         //     sine[i] -= prev_sine[i];
//         // }
//         auto& vec =  codec.emplace_back();

//         for (int i = 0; i < sine.size(); ++i)
//         {
//             auto code = static_cast<int>( sine[i] * range ) -  static_cast<int>(prev_sine[i] * range );
//             vec.push_back(code);
//             //std::cout << std::setw(3) << code << " ";
//         }
//         //std::cout << std::endl;
//         prev_sine = sine;
//         sine.clear();
//     }


//     int range = 32767;
//     int sign = 1;
//     int cross_ziro_count = 0;
// //    int mixin = 0;

//     std::vector<std::vector<int>> codec;
//     std::vector<float> sine;
//     std::vector<float> prev_sine;
// };





int main(int argc, char**argv)
{

try
{
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
    profiling::timed execution_time;
    if (!args.has('d'))
    {
        DMCModelConfig config;
        config.threshold = 4;
        config.bigthresh = 40;
        config.reset_on_overflow = false;
        config.maxnodes = 1ULL << 23;

        DMCModel model_L(config);
        DMCModel model_R(config);

        DMC_compressor_ref_model enc(&model_L);

        auto sample =  reinterpret_cast<const float*>( mmap.data() );
        bool opt_mono = args.has("mono");
        size_t samples_nb = mmap.size() / 4 / (opt_mono ? 1 : 2);
        enc.put_symbol_bits<64>(samples_nb);
        int opt_bits = args.get("bits",16,4,30);
        enc.put_symbol(opt_bits,0);
        float float_to_int_multipler = 1 << (opt_bits-1);

        // DPCMQ_codec_int DPCMQ_L(opt_bits);
        // DPCMQ_codec_int DPCMQ_R(opt_bits);
        std::cout << "total samples: " << samples_nb << std::endl;
        // enc.set_model(&model_L);
        std::vector<vector_coding> vc( opt_mono ? 1 : 2);

        auto n = samples_nb;
        while (n--)
        {
            // enc.put_symbol_bits<3>( DPCMQ_L.encoder( *sample++ * float_to_int_multipler) );
            // enc.put_symbol_bits<3>( DPCMQ_R.encoder( *sample++ * float_to_int_multipler) );
            #if 0
            auto ch0 = DPCMQ_L.encoder( std::clamp<float>(*sample++,-1.0, 1.0) * float_to_int_multipler );
            auto ch1 = DPCMQ_R.encoder( std::clamp<float>(*sample++,-1.0, 1.0) * float_to_int_multipler );
            enc.put_symbol(ch0,1);
            enc.put_symbol(ch1,1);
            #endif
            for (auto & _vc : vc)
                _vc.encode( std::clamp<float>(*sample++,-1.0, 1.0) );

        }

        for (auto & _vc : vc)
        {
            _vc.flush();
            size_t len_test = 0;

            for (auto & vec : _vc.get_codes())
            {
                //assert(codes[i] >= 0);
                //assert(codes[i+1] >= 0);
                enc.set_model( &model_L);
                len_test += vec.size();


                enc.put_symbol(vec.size() ,0);

                enc.set_model( &model_R);
                for ( auto val : vec ) {
                   enc.put_symbol(val,1);
                }
            }
            std::cout << len_test << " expected " << samples_nb << std::endl;
        }
        auto encoded = enc.finish();

        // DPCMQ_L.show();
        // DPCMQ_R.show();

        std::cout << std::endl;
        std::cout << "nodes: " << model_L.get_nodes_count() <<  "/" << model_R.get_nodes_count() << std::endl;
        std::cout << "compressed size: " << encoded.size() << std::endl;
        std::string output_filename =  ( args.size() > 2 ) ? args[2] :  args[1] + ".sdmc";
        std::ofstream output(output_filename, std::ios::binary);
        if (!output) {
            std::cerr << "can not create output file!!!" << std::endl << "ERROR (" << errno << ") : " <<  strerror(errno) << std::endl;
            return 1;
        }
        output.write(reinterpret_cast<const char*>(encoded.data()),encoded.size());
    }
    else
    {
        DMCModelConfig config;
        config.threshold = 4;
        config.bigthresh = 40;
        config.reset_on_overflow = false;
        config.maxnodes = 1ULL << 23;


        DMCModel model_L(config);
        DMCModel model_R(config);
        DMC_decompressor_ref_model dec(mmap.cbegin(), mmap.cend(),&model_L);

        auto blocks_nb = dec.get_symbol_bits<64>();
        int opt_bits = dec.get_symbol(0);
        float float_to_int_multipler = 1 << (opt_bits-1);

        DPCMQ_codec_int DPCMQ_L(opt_bits);
        DPCMQ_codec_int DPCMQ_R(opt_bits);

        std::string output_filename =  ( args.size() > 2 ) ? args[2] :  args[1] + ".f32.raw";
        auto outsize = blocks_nb*2*4;

        createEmptyFile(output_filename,outsize);

        auto rw_mmap = mio::make_mmap<mio::ummap_sink>(output_filename, 0, outsize, error);
        if (error)
        {
            std::cout << error.message() << std::endl;
            return 1;
        }
        auto sample =  reinterpret_cast<float*>( rw_mmap.data() );


        int sign = 1;
        int delta = 0;
        int prev_sample = 0;
        size_t samples_nb=0;
        int channel = 0;

        while ( channel < 2 )
        {
            // int ch0 = DPCMQ_L.decoder( dec.get_symbol(1) );
            // int ch1 = DPCMQ_R.decoder( dec.get_symbol(1) );
            dec.set_model(&model_L);
            int diff = dec.get_symbol(1);
            dec.set_model(&model_R);
            int length = dec.get_symbol(0);


                int new_sample = std::clamp(prev_sample + tinyint_to_int(diff), -32767, 32767 );
                //sign = -sign;
                samples_nb += length;
                if ( samples_nb > blocks_nb) {
                    std::cout << "lost sync!!!" << std::endl;
                    return 1;
                }
                for (int i = 0; i < length; ++i) {

                    *sample = linear(i+1,length+1,prev_sample, new_sample ) / float_to_int_multipler;
                    sample += 2;

                }
                prev_sample = new_sample;
                if ( samples_nb == blocks_nb) {
                    channel++;
                    sample =  reinterpret_cast<float*>( rw_mmap.data() ) + channel;
                    samples_nb = 0;
                    sign = 1;
                    delta = 0;
                    prev_sample = 0;
                }

            // *sample++ = std::clamp<float>( ch0 / float_to_int_multipler, -1.0, 1.0);
            // *sample++ = std::clamp<float>( ch1 / float_to_int_multipler, -1.0, 1.0);
        }

    }

    // #if 0
    //     //encoder
    //     DMCModelConfig config;
    //     config.threshold = 4;
    //     config.bigthresh = 40;
    //     config.reset_on_overflow = false;
    //     config.maxnodes = 1ULL << 23;
    //     DMC_compressor enc(config);
    //     //ADPCM4_codec codec;
    //     DPCMQ_codec left_codec;
    //     DPCMQ_codec right_codec;
    //         auto sample =  reinterpret_cast<const float*>( mmap.data() );
    //         size_t samples_nb = mmap.size() / 4 / 2;
    //         enc.put_symbol_bits<64>( samples_nb );

    //         while (samples_nb--)
    //         {
    //             enc.put_symbol( left_codec.encoder( *sample++), 1);
    //             enc.put_symbol( right_codec.encoder( *sample++), 1);
    //         }

    //     auto encoded = enc.finish();

    //     std::cout << std::endl;
    //     std::cout << "nodes: " << enc.get_nodes_count() << std::endl;
    //     std::cout << "compressed size: " << encoded.size() << std::endl;
    //     std::string output_filename =  ( args.size() > 2 ) ? args[2] :  args[1] + ".sdmc";
    //     std::ofstream output(output_filename, std::ios::binary);
    //     if (!output) {
    //         std::cerr << "can not create output file!!!" << std::endl << "ERROR (" << errno << ") : " <<  strerror(errno) << std::endl;
    //         return 1;
    //     }
    //     output.write(reinterpret_cast<const char*>(encoded.data()),encoded.size());

    //     //decoder
    //     DMCModelConfig config;
    //     config.threshold = 4;
    //     config.bigthresh = 40;
    //     config.reset_on_overflow = false;
    //     config.maxnodes = 1ULL << 23;
    //     DMC_decompressor dec(mmap.cbegin(), mmap.cend(), config);

    //     DPCMQ_codec left_codec;
    //     DPCMQ_codec right_codec;
    //     size_t samples_nb = dec.get_symbol_bits<64>();

    //     size_t outsize = samples_nb * 4 * 2;

    //     std::string output_filename =  ( args.size() > 2 ) ? args[2] :  args[1] + ".ulaw.raw";

    //     createEmptyFile(output_filename,outsize);

    //     auto rw_mmap = mio::make_mmap<mio::ummap_sink>(output_filename, 0, outsize, error);
    //     if (error)
    //     {
    //         std::cout << error.message() << std::endl;
    //         return 1;
    //     }
    //     auto sample =  reinterpret_cast<float*>( rw_mmap.data() );

    //     while (samples_nb--)
    //     {
    //         *sample++ = left_codec.decoder( dec.get_symbol(1) );
    //         *sample++ = right_codec.decoder( dec.get_symbol(1) );
    //     }
    // #endif

} catch (const std::exception & e) {
    std::cerr << e.what() << std::endl;
}

}