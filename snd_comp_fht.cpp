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
#include "dwt97.hpp"

using myargs::Args;
using pack::DMC_compressor_ref_model;
using pack::DMC_decompressor_ref_model;
using pack::DMCModelConfig;
using pack::DMCModel;
Args args;


/* a simple recursive in-place fast hadamard transform */
/* x is the input vector, and n is the size, a power of two */
template <typename T>
void fht(T * x, size_t n)
{
  /* stopping case */
  if (n == 1)
    return;

  /* recurse */
  fht(x, n/2);
  fht(x+n/2, n/2);

  /* butterflies */
  int i;
  for (i = 0 ; i < n/2 ; i++)
  {
    T tmp = x[i];
    x[i] = tmp + x[n/2+i];
    x[n/2+i] = tmp - x[n/2+i];
  }

  return;
}


#include <complex>
#include <cmath>

//typedef std::complex<float> complex_t;

template <typename complex_t>
void fft0(int n, int s, bool eo, complex_t* x, complex_t* y)
// n  : sequence length
// s  : stride
// eo : x is output if eo == 0, y is output if eo == 1
// x  : input sequence(or output sequence if eo == 0)
// y  : work area(or output sequence if eo == 1)
{
    using T = decltype(x->real());
    const int m = n/2;

    const T theta0 = T(2)*M_PI/n;

    if (n == 2) {
        complex_t* z = eo ? y : x;
        for (int q = 0; q < s; q++) {
            const complex_t a = x[q + 0];
            const complex_t b = x[q + s];
            z[q + 0] = a + b;
            z[q + s] = a - b;
        }
    }
    else if (n >= 4) {
        for (int p = 0; p < m; p++) {
            const complex_t wp = complex_t(std::cos(p*theta0), -std::sin(p*theta0));
            for (int q = 0; q < s; q++) {
                const complex_t a = x[q + s*(p + 0)];
                const complex_t b = x[q + s*(p + m)];
                y[q + s*(2*p + 0)] =  a + b;
                y[q + s*(2*p + 1)] = (a - b) * wp ;
            }
        }
        fft0(n/2, 2*s, !eo, y, x);
    }
}

template <typename T>
void dht(T * begin, int n)
{

    std::vector<std::complex<T>> y(n);
    std::vector<std::complex<T>> x(begin, begin + n);

    fft0(n, 1, 0, x.data(), y.data());
    for (int i=0; i<n; ++i)
    {
        begin[i] = x[i].real() - x[i].imag();
    }
}

template <typename T>
void idht(T * begin, int n)
{
    dht(begin, n);
    float scale = T(1) / n;
    for (int i=0; i < n; ++i)
    {
        begin[i] *= scale;
    }
}


/*

    // // Находим абсолютный максимальный элемент
    // auto volume = std::abs(*std::max_element(csine, csine+block_size, [](auto a, auto b) {
    //     return std::abs(a) < std::abs(b);
    // }));

    // auto volume_integer = int(volume * 32767.0f);
    // enc.put_symbol(volume_integer,0);

    //std::cout << "volume = " << volume << std::endl;

    // if ( volume_integer == 0 ) {
    //     return;
    // }


    // auto sine = std::vector<float>(csine, csine+block_size);
    // for (int i=0; i<block_size; i++) {
    //     sine[i] /= volume;
    // }
    //std::cout << sine << std::endl;
    // std::vector<float> sine(csine, csine + block_size);

    // dht(sine.data(), sine.size());

    // int power = tinyint_to_int(  std::max<int>(1, std::abs( int_to_tinyint( int(*std::max_element(sine.begin(), sine.end(), [](auto a, auto b) {
    //     return std::abs(a) < std::abs(b);
    // })))) - 4));

    //std::cout << "power = " << tinyint_to_int(power) << std::endl;
    //std::vector<float> sine_packed(sine.size());
    //auto max_power = *std::max_element(sine.begin(), sine.end()) / q;




    // for (int i = 0; i<block_size; i++) {

    //         enc.put_symbol( round( sine[i] / block_size * q) , 1);
    // }

    //std::cout << "packed spactr: " << std::endl;
    //std::cout << sine_packed << std::endl;

*/

void encode_block(DMC_compressor_ref_model &enc, DMCModel* base_model, DMCModel* scale_model,  const float * csine, int block_size, int q=32 )
{


    std::vector<float> work_area( block_size );
    std::vector<float> block_int( csine, csine + block_size );


    if (q == 0)
        q = 16;
    if (q > 256)
        q = 256;


    for (int size = block_size; size>=2;  size/=2) {

        dwt97(block_int.data(), work_area.data(), size);

        // int avg = 0;
        // for (int i=size/2; i < size; ++i)
        // {
        //     avg += std::abs(block_int[i]);
        // }
        // avg /= size / 2 ;


        // if (args.has("trace")) std::cerr << size/2 << " [" << avg  << "] :" ;



        for (int i=size/2; i < size; ++i)
        {

            enc.put_symbol( block_int[i] * q, 1);

        }

    }
    enc.put_symbol(block_int[0] * q,1);


    // for (int size=2; size <= block_int.size(); size *= 2)
    // {
    //     auto max_element = std::abs(*std::max_element( block_int.begin() + size/2, block_int.begin() + size, [](int a, int b) { return std::abs(a) < std::abs(b); } ));
    //     enc.set_model(base_model);
    //     enc.put_symbol( max_element, 0);
    //     enc.set_model(scale_model);
    //     if ( max_element > 0) {
    //         for (int i = 0; i < size /2; i++)
    //         {
    //             enc.put_symbol(  round(block_int[i+size/2]  * q / float(max_element))   , 1);
    //         }
    //     }
    // }
}

template <typename DMC_decompressor>
void decode_block(DMC_decompressor & dec,  DMCModel* base_model, DMCModel* scale_model,  float * output, int block_size, int q=32 )
{


    std::vector<float> work_area(block_size,0);
    // for (int size=2; size <= block_size; size *= 2)
    // {
    //     dec.set_model(base_model);
    //     int max_element = dec.get_symbol(0);


    //     if (max_element > 0) {
    //         dec.set_model(scale_model);
    //         //max_element = (1 << (max_element-1));
    //         auto max_element_float =  float(max_element) / float(q);
    //         for (int i = 0; i< size /2; i++)
    //         {
    //             block_int[i+size/2] = round(dec.get_symbol(1) * max_element_float);
    //         }
    //     }
    // }

    for (int size = block_size; size>=2;  size/=2) {
        for (int i = size/2; i < size; ++i)
        {
            output[i] = dec.get_symbol(1) / float(q);
            //std::cerr << block_int[i] << " ";
        }
        //std::cerr << std::endl;
    }
    output[0] =  dec.get_symbol(1) / float(q);
     //std::cerr << block_int[0] << std::endl << std::endl;

    for (int size = 2; size <= block_size; size*=2) {
        idwt97(output, work_area.data(), size );
    }

}

constexpr int SIGNATURE = 0x11223305;

void createEmptyFile(const std::string& fileName, std::streamsize fileSize) {
    FILE *fp=fopen(fileName.data(), "w");
    _chsize_s(fileno(fp),fileSize);
    fclose(fp);
}


double sinc(double x) {
  if (fabs(x)<1e-6) return 1.0;
  return std::sin(M_PI * x)/(M_PI * x);
}

float mix_audio( float signal_1, float signal_2 )
{
    auto sum = signal_1 + signal_2;
    auto sign = sum < 0 ? -1.0 : 1.0;
    return sum - (signal_1 * signal_2 * sign);

}

float linear(float x, float width, float x0, float x1)
{
    return (x1-x0) / width * x + x0;
}

template <typename T>
bool intersection( T a_min, T a_max, T b_min, T b_max )
{
    return std::max DUMMY (a_min, b_min) < std::min DUMMY (a_max, b_max);
}

void glue_blocks(float* sf32, int block_size) {


    // //auto predict_diff = ((sf32[-1] - sf32[-2]) + (sf32[1] - sf32[0])) / 2.0f;
    // //auto predict_diff =  std::M (sf32[0] + sf32[1] + sf32[2] + sf32[3]) / 4 - (sf32[-1] + sf32[-2]) + (sf32[-3] + sf32[-4]) / 4;
    // auto  prev_max = *std::max_element( sf32-8, sf32);
    // auto  prev_min = *std::min_element( sf32-8, sf32);
    // auto  cur_max = *std::max_element( sf32, sf32+8);
    // auto  cur_min = *std::min_element( sf32, sf32+8);
    // if ( intersection(prev_min, prev_max, cur_min, cur_max) )
    //     return;

    // auto diff =  (cur_max + cur_min)/2 - (prev_max + prev_min) / 2;

    // for (int i=0; i<16; ++i)
    // {

    //     auto step = linear(i,16, diff, 0);
    //     if (std::abs(step)  < 0.00005f)
    //         return;
    //     sf32[i] -= diff;
    // }

}


int main(int argc, char** argv)
{

    try {
    args.parse(argc, argv);

    DMCModelConfig config;
    config.threshold = 4;
    config.bigthresh = 40;
    config.reset_on_overflow = false;
    config.maxnodes = 1ULL << 23;

    int block_size = 1 << ( ilog2_32( args.get("block-size",256,64,32768),1) -1 ) ;

    std::error_code error;
    mio::ummap_source mmap = mio::make_mmap<mio::ummap_source>(args[1], 0, 0, error);
    if (error)
    {
        std::cout << error.message() << std::endl;
        return 1;
    }

    if ( args.has('d') ) {
        DMCModel base_model(config);
        DMCModel scale_model(config);

        auto dec = DMC_decompressor_ref_model( mmap.cbegin(), mmap.cend(), &base_model );
        if ( dec.get_symbol(0) != SIGNATURE ) {
            std::cerr << "input is not compressed by fht-1" << std::endl;
            return 2;
        }
        block_size = 1 << dec.get_symbol(0);
        int q = dec.get_symbol(0);
        int blocks_nb = dec.get_symbol(0);
        std::cout << "block-size = " << block_size << std::endl;
        std::cout << "q = " << q << std::endl;
        size_t outsize = (blocks_nb) * size_t(block_size*4);
        std::string output_filename =  ( args.size() > 2 ) ? args[2] :  args[1] + ".f32.raw";

       createEmptyFile(output_filename,outsize);

        auto rw_mmap = mio::make_mmap<mio::ummap_sink>(output_filename, 0, outsize, error);
        if (error)
        {
            std::cout << error.message() << std::endl;
            return 1;
        }
        auto sf32 = reinterpret_cast<float*>( rw_mmap.data() );


        for( int n = 0; n < blocks_nb; ++n )
        {
            decode_block(dec, &base_model, &scale_model, sf32, block_size,q);
            if (n > 0) {
                glue_blocks(sf32, block_size);
            }
            //clip
            for (int i = 0; i < block_size; i++)
            {

                sf32[i] = std::clamp<float>( sf32[i], -1.0f, 1.0f);

            }
            sf32 += block_size;
        }

        return 0;
    }
    auto sf32 = reinterpret_cast<const float*>(  mmap.data() );
    auto block_nb = mmap.size() / (block_size * 4 );


    DMCModel base_model(config);
    DMCModel scale_model(config);

    DMC_compressor_ref_model enc(&base_model);
    int prev_sample = 0;
    int  q_opt = args.get('q',128,1,1024);
    enc.put_symbol(SIGNATURE,0); //signature
    enc.put_symbol( ilog2_32( block_size,1 )-1, 0 );
    enc.put_symbol( q_opt, 0);
    std::cout << "block-size = " << block_size << std::endl;
    std::cout << "q = " << q_opt << std::endl;
    enc.put_symbol(block_nb,0); //blocks count

    for (size_t n = 0; n < block_nb; ++n)
    {
        encode_block(enc,&base_model, &scale_model, sf32,block_size, q_opt);
       sf32 += block_size;

    }

    std::string output_filename =  ( args.size() > 2 ) ? args[2] :  args[1] + ".sfht";
    std::ofstream output(output_filename, std::ios::binary | std::ios::trunc);
    auto bytes = enc.finish();
    output.write(  reinterpret_cast<const char*>( bytes.data() ), bytes.size() );
    } catch (std::exception & e) {
        std::cerr << e.what() << std::endl;
    }

}