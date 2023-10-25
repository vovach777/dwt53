#include <string>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include "myargs/myargs.hpp"
#include "the_matrix.hpp"
#include "img_load.hpp"
#include "bwt_mtf_rle.hpp"
#include "quantization.hpp"


    using myargs::Args;
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;


inline std::vector<uint8_t> compress(std::vector<the_matrix>& planes) {

    std::vector<uint8_t> data;
    // data.push_back( planes.size() ); //channels
    // data.push_back( planes[0][0].size() ); //width
    // data.push_back( planes[0].size());  //height

    size_t packed_size = 0;
    size_t data_size = 0;
    size_t row_count = 0;
    for (auto & plane : planes) {
        row_count += plane.size();
    }

    size_t row_nb = 0;
    for (auto& plane : planes ) {
        for (auto &row : plane )
        {
            auto quant = scalar_adaptive_quantization(row.begin(), row.end(),-32,256,std::vector<int>{});
            for (auto &val : row )
                val = quant[val];
            auto index = bwt_encode(row.begin(), row.end());
            //data.push_back(index);
            rlep_encode(row.begin(), row.end(),data);
            // for (auto it = data.begin(); it != data.end(); ++it) {
            //     std::cout << (int)*it << " ";
            // }
            // std::cout << std::endl;
            row_nb++;
            // if (row_nb == 8)
            //     return data;
            data_size += row.size();
            std::cout << "\r" << (data_size) << " -> " << ((data.size()) ) << " " << (row_nb*100 / row_count) << "        \b\b\b\b\b\b\b\b";
        }
    }

    // for (auto & row : matrix)
    // for (auto & val : row)
    //     enc.put_symbol(std::clamp<int>(val,0,255),0);
    //     //enc.put_symbol_bits<8>( std::clamp<int>(val,0,255));

    return data;
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
    auto packed_size = compress(planes).size();
    std::cout << std::endl;
    std::cout << "compressed size : " << packed_size << std::endl;

}
