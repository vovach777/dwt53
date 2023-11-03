#define NOMINMAX
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
#include <array>
#include <variant>
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
#include "dkm.hpp"
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


class MSE {
    public:
    void update(float ideal, float aproxymated) {
        float error = aproxymated - ideal;
        sum_sq += (error * error);
        count++;
    }
    float value() {
        return sum_sq / count;
    }
    void reset() {
        sum_sq = 0.0;
        count = 0;
    }
    private:
    float sum_sq = 0.0;
    size_t count = 0;
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

    prev_sample = std::clamp( prev_sample, -32767, 32767 );
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

    prev_sample = std::clamp(predictor,-32767, 32767);

    return prev_sample;

}

};


class diff_codec {
    public:
    diff_codec()
    {

    }
    int encode(float sample) {


        auto pair = adpcm_cb.lockup_pair( round( (sample - prev_sample) * 256.0f) );
        prev_sample = std::clamp( prev_sample  + pair.second * float(1.0/256.0) , -1.0f, 1.0f );

        //std::cout << sample << " -> " << pair.first << std::endl;

        return pair.first;
    }
    float decode(int diff) {
        return prev_sample = std::clamp( prev_sample + adpcm_cb.get_codebook_value_at(diff) * float(1.0/256.0), -1.0f, 1.0f );
    }
    private:
        float prev_sample = 0.0f;

        Codebook adpcm_cb = Codebook( std::vector<int>{
           //0, 60, 195, 501, 1190, 2742, 6240, 14123, 31887
           //0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 14, 17, 23, 34, 149
           0,1,2,3,4,6,12,136
        });
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

    DPCMQ_codec_int(int bits)
    {
        clip_min = -(1 << (bits-1));
        clip_max =  (1 << (bits-1));
    }

int encoder(int sample)
{
    int diff = findFibonacciIndex( sample - prev_sample);

    prev_sample = std::clamp( prev_sample + getFibonacciNumber(diff), clip_min, clip_max );
    return diff;
}

int decoder(int diff)
{
    prev_sample = std::clamp(  prev_sample + getFibonacciNumber( diff ), clip_min, clip_max );
    return prev_sample;
}

};



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
        bool opt_mono = args.has("mono");
        int channels_nb = opt_mono ? 1 : 2;
        auto sample =  reinterpret_cast<const float*>( mmap.data() );
        size_t samples_nb = mmap.size() / 4 / channels_nb;



        int discretize = args.get("discretize",1024,1 << 8,1 << 24);
        std::vector<size_t> vectors(discretize);

        if (args.has("stat")) {
            std::cout << "stat N=" << samples_nb << " discretize=" << discretize << std::endl;
            auto sample =  reinterpret_cast<const float*>( mmap.data() );
            float prev = 0.0f;
            for (size_t n=0; n < samples_nb; ++n)
            {
                vectors.at( std::min DUMMY (discretize-1,  int(std::abs(*sample - prev) * discretize)) )++;
                prev = *sample;
                sample += channels_nb;
            }
            std::vector<std::array<double, 2>> dataset;
            for (int i = 0; i < discretize; ++i) {
                size_t count = vectors.at(i);
                if (count > 0 /* samples_nb/discretize/8 */ ) {
                    auto & array = dataset.emplace_back();
                    array[0] = double(i);
                    array[1] = double(count);
                }
            }
            std::cout << "dataset size=" << dataset.size() << std::endl;
            auto cluster_data = dkm::kmeans_lloyd(dataset,args.get("size",16,2,1024) );
            std::cout << "Means:" << std::endl;
            std::vector<int> values;
            for (const auto& mean : std::get<0>(cluster_data)) {
                std::cout << "\t(" << int(mean[0]) << "," << int( mean[1]) << ")" << std::endl;
                values.push_back(mean[0]);
            }
            std::sort(values.begin(), values.end());
            for (auto v : values)
                std::cout << v << ", ";
            std::cout << std::endl;
            //std::cout << "\nCluster labels:" << std::endl;
            // std::cout << "\tPoint:";
            // for (const auto& point : dataset) {
            //     std::stringstream value;
            //     value << "(" << point[0] << "," << point[1] << ")";
            //     std::cout << std::setw(14) << value.str();
            // }
            // std::cout << std::endl;
            // std::cout << "\tLabel:";
            // for (const auto& label : std::get<1>(cluster_data)) {
            //     std::cout << std::setw(14) << label;
            // }
            return 0;
        }

        if (args.has("vec")) {
            std::cout << "vec N=" << samples_nb << " discretize=" << discretize << std::endl;
            auto sample =  reinterpret_cast<const float*>( mmap.data() );
            float prev = 0.0f;
            std::vector<std::array<float, 8>> dataset;

            for (size_t n=0; n < samples_nb*channels_nb-8 &&  dataset.size() < discretize; n+=8)
            {
                auto& array = dataset.emplace_back();
                std::copy(sample, sample+8, std::begin(array));
                sample+=8;

            }

            std::cout << "dataset size=" << dataset.size() << std::endl;
            auto cluster_data = dkm::kmeans_lloyd(dataset,args.get("size",16,2,1024),dataset.size()*2,1.0f/1024.0f );
            auto & means = std::get<0>(cluster_data);
            std::cout << "Means:" << std::endl;

            for (const auto& mean : means ) {

                std::cout << "\t(";
                for (auto v : mean)
                    std::cout << int(v*1024) << ",";
                std::cout << "\b)" << std::endl;
            }
            std::cout << std::endl;

            std::string output_filename =  args[1] + ".vec.raw";
            auto outsize = mmap.size();

            createEmptyFile(output_filename,outsize);

            auto rw_mmap = mio::make_mmap<mio::ummap_sink>(output_filename, 0, outsize, error);
            if (error)
            {
                std::cout << error.message() << std::endl;
                return 1;
            }

            auto sample_in =  reinterpret_cast<const float*>( mmap.data() );
            auto sample_out =  reinterpret_cast<float*>( rw_mmap.data() );



            for (const float* sample = sample_in; sample < sample_in + (mmap.size() / 4) -8; sample += 8, sample_out += 8)
            {

                std::array<float, 8> array;
                std::copy(sample, sample+8, std::begin(array));
                auto index = dkm::details::closest_mean( array, means);
                std::copy( std::begin( means[index] ),  std::end( means[index] ), sample_out );
                if (trace_opt) {
                    for (auto v :  means[index]  )
                        std::cout << int(v*1024) << ",";
                    std::cout << std::endl;
                }
            }


            //std::cout << "\nCluster labels:" << std::endl;
            // std::cout << "\tPoint:";
            // for (const auto& point : dataset) {
            //     std::stringstream value;
            //     value << "(" << point[0] << "," << point[1] << ")";
            //     std::cout << std::setw(14) << value.str();
            // }
            // std::cout << std::endl;
            // std::cout << "\tLabel:";
            // for (const auto& label : std::get<1>(cluster_data)) {
            //     std::cout << std::setw(14) << label;
            // }
            return 0;
        }


        DMCModelConfig config;
        config.threshold = 4;
        config.bigthresh = 40;
        config.reset_on_overflow = false;
        config.maxnodes = 1ULL << 23;


        DMC_compressor enc(config);

        enc.put_symbol_bits<64>(samples_nb);
        int opt_bits = args.get("bits",16,16,16);
        enc.put_symbol(opt_bits,0);
        enc.put(opt_mono);

        float float_to_int_multipler = (1 << (opt_bits-1)) - 1;

        std::vector<diff_codec> codecs;
        for (int i=0; i < channels_nb; ++i)
            codecs.emplace_back();

        std::cout <<  samples_nb << " samples, " <<(opt_mono ? " mono" : " stereo") << ", " << opt_bits << " bits." << std::endl;


        for (size_t n=0; n < samples_nb; ++n)
        {
            for (auto & codec : codecs ) {
                enc.put_symbol( codec.encode( *sample++) ,1);
                //enc.put_symbol( codec.encode( std::clamp<float>(*sample,-1.0f, 1.0f) * float_to_int_multipler ) ,1);
                //enc.put_symbol( codec.encode( std::clamp<float>(*sample,-1.0f, 1.0f) * float_to_int_multipler ) ,1);
            }

        }

        auto encoded = enc.finish();

        std::cout << std::endl;
        std::cout << "nodes: " << enc.get_nodes_count() << std::endl;
        std::cout << "compressed size: " << encoded.size() << std::endl;
        std::string output_filename =  ( args.size() > 2 ) ? args[2] :  args[1] + ".sdmc2";
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

        DMC_decompressor dec(mmap.cbegin(), mmap.cend(), config);

        auto samples_nb = dec.get_symbol_bits<64>();
        int opt_bits = dec.get_symbol(0);
        float float_to_int_multipler = 1.0f / ( (1 << (opt_bits-1))-0 );
        bool opt_mono = dec.get();
        int channels_nb = opt_mono ? 1 : 2;
        std::cout << samples_nb << " samples"  << (opt_mono ? ", mono" : ", stereo") << ", " << ( opt_bits ) << " bits." << std::endl;

        std::vector<diff_codec> codecs;
        for (int i = 0; i < channels_nb; ++i)
            codecs.emplace_back();

        std::string output_filename =  ( args.size() > 2 ) ? args[2] :  args[1] + ".f32.raw";
        auto outsize = samples_nb*4*channels_nb;

        createEmptyFile(output_filename,outsize);

        auto rw_mmap = mio::make_mmap<mio::ummap_sink>(output_filename, 0, outsize, error);
        if (error)
        {
            std::cout << error.message() << std::endl;
            return 1;
        }
        auto sample =  reinterpret_cast<float*>( rw_mmap.data() );


        for (size_t n = 0; n <  samples_nb; ++n)
        {
            for (auto & codec : codecs) {
                *sample++ = codec.decode( dec.get_symbol(1) );
                //*sample++ = std::clamp<float>( codec.decode( dec.get_symbol(1) ) * float_to_int_multipler, -1.0f, 1.0f);
                // int a = codec.decode( dec.get_symbol(1) );
                // int b = codec.decode( dec.get_symbol(1) );
                // *sample++ = std::clamp<float>( ((a + b) / 2) * float_to_int_multipler, -1, 1);
            }


        }

    }


} catch (const std::exception & e) {
    std::cerr << e.what() << std::endl;
}

}