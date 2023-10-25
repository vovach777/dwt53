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
#include "dwt53.hpp"

using myargs::Args;
using pack::DMC_compressor;
using pack::DMC_decompressor;
using pack::DMCModelConfig;
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


void encode_block(DMC_compressor &enc, const float * csine, int block_size, int q=32 )
{
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


    std::vector<int> work_area( block_size );
    for (int i=0; i< block_size; ++i)
        work_area[i] = csine[i]*32767.0;


    std::vector<int> block_int( block_size );

    for (int size = block_size; size>=2;  size/=2) {

        dwt53(block_int.data(), size, work_area.data(), work_area.data() + size/2);
        std::copy(work_area.begin(), work_area.begin() + size, block_int.begin());

    }


    for (int size=2; size <= block_int.size(); size *= 2)
    {
    for (int i = 0, max_element = std::abs(*std::max_element( block_int.begin() + size/2, block_int.begin() + size, [](int a, int b) { return std::abs(a) < std::abs(b); } )); i< size /2; i++)
    {
        if (i == 0)
            enc.put_symbol(max_element,0);

        enc.put_symbol( round(block_int[i+size/2] / float(max_element) *  q) , 1);
    }
    }
}

template <typename DMC_decompressor>
void decode_block(DMC_decompressor & dec, float * output, int block_size, int q=32 )
{

    for (int i = 0; i<block_size; ++i)
    {
        output[i] = dec.get_symbol(1) / float(q) * block_size;
    }
    //inverse transform
    idht(output, block_size);
}

constexpr int SIGNATURE = 0x11223302;

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


void glue_block(float * block, int block_size)
{

//   std::fill( block, block+block_size, 0.0f);
//   block[0] = -1;

    float phase_2 = 0;
    for (int i = 0; i < 4; i++) {
        phase_2 += block[-i];
    }

    float phase_1 = 0;
    for (int i = 0; i < 4; i++) {
        phase_1 += block[-i-8];
    }

    float phase_3 = 0;
    for (int i = 0; i < 4; i++) {
        phase_3 += block[i];
    }

    float phase_4 = 0;
    for (int i = 0; i < 4; i++) {
        phase_4 += block[i+8];
    }


    if ( ( phase_1 <= phase_2 && phase_3 <= phase_4 ) ||
         (phase_1 >= phase_2 && phase_3 >= phase_4 ) )
        {
            // do nothing
    } else {
        //inverse_phase
        // for (int i=0; i < block_size; i++)
        //    block[i] *= -1;
    }

    float diff = block[-1] - block[0];

    for (int i = 0; i< block_size; i++)
    {
       block[i] = std::clamp<float>(  block[i] +  linear(i+1, block_size,  diff, 0), -1.0f, 1.0f);
    }

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

    int block_size = 1 << ( ilog2_32( args.get("block-size",256,64,8192),1) -1 ) ;

    std::error_code error;
    mio::ummap_source mmap = mio::make_mmap<mio::ummap_source>(args[1], 0, 0, error);
    if (error)
    {
        std::cout << error.message() << std::endl;
        return 1;
    }

    if ( args.has('d') ) {
        auto dec = DMC_decompressor( mmap.cbegin(), mmap.cend(), config );
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
            decode_block(dec, sf32, block_size,q);
            if (n > 0 ) {
                glue_block(sf32, block_size);
            }
            sf32 += block_size;
        }

        return 0;
    }
    auto sf32 = reinterpret_cast<const float*>(  mmap.data() );
    auto block_nb = mmap.size() / (block_size * 4 );



    DMC_compressor enc(config);
    int prev_sample = 0;
    bool put_symbol_opt = args.has("symbol");
    int  q_opt = args.get('q',32,1,block_size*2);
    enc.put_symbol(SIGNATURE,0); //signature
    enc.put_symbol( ilog2_32( block_size,1 )-1, 0 );
    enc.put_symbol( q_opt, 0);
    std::cout << "block-size = " << block_size << std::endl;
    std::cout << "q = " << q_opt << std::endl;
    enc.put_symbol(block_nb,0); //blocks count

    while (block_nb--)
    {
        encode_block(enc,sf32,block_size, q_opt);
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