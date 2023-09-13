#include <string>
#include <iomanip>
#include "dmc.hpp"
#include <chrono>
#include "myargs/myargs.hpp"
#include "the_matrix.hpp"
#include "img_load.hpp"

    using myargs::Args;
    using pack::DMC_decompressor;
    using pack::DMCModelConfig;
    using pack::DMC_compressor;
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;


inline std::vector<uint8_t> compress(const the_matrix& matrix) {

    DMCModelConfig config;
    config.threshold = 4;
    config.bigthresh = 40;
    config.reset_on_overflow = false;
    config.maxnodes = 1ULL << 23;
    DMC_compressor enc(config);


    enc.put_symbol(matrix[0].size(),0); //width
    enc.put_symbol(matrix.size(),0);  //height

    for (auto & row : matrix)
    for (auto & val : row)
        enc.put_symbol(std::clamp<int>(val,0,255),0);
        //enc.put_symbol_bits<8>( std::clamp<int>(val,0,255));

    return enc.finish();
}

template <typename Iterator>
inline the_matrix decompress(Iterator begin, Iterator end) {

    DMCModelConfig config;
    config.threshold = 4;
    config.bigthresh = 40;
    config.reset_on_overflow = false;
    config.maxnodes = 1ULL << 23;

    DMC_decompressor dec(begin, end, config);

    auto width = dec.get_symbol_bits<24>();
    auto height = dec.get_symbol_bits<24>();

    the_matrix matrix(height, std::vector<int>(width));
    for (auto & row : matrix)
    for (auto & val : row)
        val = dec.get_symbol_bits<8>();

    return matrix;

}


int main(int argc, char **argv)
{
    Args args;
    args.parse(argc, argv);
    if (args.size() < 2) {
        std::cout << "usage: \tprog  <image>\n";
        return 1;
    }
    auto planes = load_image_as_yuv420(args[1].data());
    size_t packed_size = 0;
    for (auto& plane : planes ) {
        packed_size += compress(plane).size();
    }
    std::cout << "compressed size : " << packed_size << std::endl;

}
