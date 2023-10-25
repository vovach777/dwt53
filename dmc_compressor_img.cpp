#include <string>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <fstream>
#include "dmc.hpp"
#include "myargs/myargs.hpp"
#include "the_matrix.hpp"
#include "img_load.hpp"
#include "mio.hpp"
#include "timed.hpp"


    using myargs::Args;
    using pack::DMC_decompressor;
    using pack::DMCModelConfig;
    using pack::DMC_compressor;
    using namespace profiling;

inline std::vector<uint8_t> compress(const std::vector<the_matrix>& planes, bool lossless, int lines_pack_log2) {

    timed comp_compresion_total_time("total component compression");
    DMCModelConfig config;
    config.threshold = 4;
    config.bigthresh = 40;
    config.reset_on_overflow = false;
    config.maxnodes = 1ULL << 23;
    DMC_compressor enc(config);
    enc.put(lossless);
    if (!lossless) {
        enc.put_symbol(lines_pack_log2,0); //power of two
    }
    enc.put_symbol(planes.size(),0); //channels nb
    enc.put_symbol(planes[0][0].size(),0); //width
    enc.put_symbol(planes[0].size(),0);  //height

    size_t packed_size = 0;
    for (auto& plane : planes ) {
        timed comp_compresion_time("component compression");
        int width = plane[0].size();
        int height = plane.size();
        if (lossless)
        {
            for (decltype( height) h=0; h<height; h++)
            for (decltype( width)  x=0; x<width; x++)
            {
                const auto left =     x        ? plane[h][x-1]   :0;
                const auto top  =     h        ? plane[h-1][x]   :0;
                const auto topleft =  (x && h) ? plane[h-1][x-1] :0;
                const auto predict = median( top, left, top + left - topleft  );
                enc.put_symbol( plane[h][x] - predict, 1 );
            }
        } else {
            int predict = 128;
            #if 0
            for (decltype( height) h=0; h<(height & ~1); h+=2)
            {
                int predict_1 = plane[h][0];
                int predict_2 = plane[h+1][0];
                enc.put_symbol( predict_1 - prev_2line, 1);
                enc.put_symbol( predict_2 - predict_1,1);
                prev_2line = predict_2;

                for (decltype( width)  x=1; x<width; x++)
                {
                    int delta = int_to_tinyint( plane[h][x] - predict_1 );
                    predict_1 = std::clamp( predict_1 + tinyint_to_int(delta), 0, 255);
                    enc.put_symbol( delta, 1 );

                    delta = int_to_tinyint( plane[h+1][x] - predict_2 );
                    predict_2 = std::clamp( predict_2 + tinyint_to_int(delta), 0, 255);
                    enc.put_symbol( delta, 1 );
                }
            }
            if (height & 1) {
                for (int x = 0; x < width; x++)
                {
                    int delta = int_to_tinyint( plane[height-1][x] - prev_2line );
                    prev_2line = std::clamp( prev_2line + tinyint_to_int(delta), 0, 255);
                    enc.put_symbol( delta, 1 );
                }
            }
            #endif
            int lines_pack = height;
            int height_truncated = height;
            if  ( (1 << lines_pack_log2) < height )
            {
                lines_pack = (1 << lines_pack_log2);
                height_truncated = height & ~(lines_pack-1);
            }

            std::vector<int> predict_lines(lines_pack);
            for (int h = 0; h < height_truncated; h+=lines_pack)
            {
                for (int i = 0; i < lines_pack; ++i)
                {
                    const int p = predict;
                    predict = predict_lines[i] = plane[h+i][0];
                    enc.put_symbol(predict - p, 1);
                }

                for (int x=1; x < width; ++x)
                for (int i=0; i<lines_pack; i++) {
                    // const int delta = int_to_tinyint( plane[h+i][x] - predict_lines[i] );
                    // predict_lines[i] = std::clamp( predict_lines[i] + tinyint_to_int(delta),0,255);
                    // enc.put_symbol( delta, 1);
                    const int delta = int_to_tinyint( plane[h+i][x] - predict_lines[i] );
                    predict_lines[i] = std::clamp( predict_lines[i] + tinyint_to_int(delta),0,255);

                }
            }
            lines_pack = ( height - height_truncated );
            if (lines_pack > 0) {
                for (int i=0; i<lines_pack; ++i) {
                    const int p = predict;
                    predict = predict_lines[i] = plane[height_truncated + i][0];
                    enc.put_symbol(predict - p, 1);
                }

                for (int x = 1; x < width; x++) {
                    for (int i=0; i<lines_pack; i++) {
                        const int delta = int_to_tinyint( plane[height_truncated + i][x] - predict_lines[i] );
                        predict_lines[i] = std::clamp( predict_lines[i] + tinyint_to_int(delta),0,255);
                        enc.put_symbol( delta, 1);
                    }
                }
            }
        }
        enc.reset_model();
    }

    return enc.finish();
}

template <typename Iterator>
inline std::vector<the_matrix> decompress(Iterator begin, Iterator end) {

    DMCModelConfig config;
    config.threshold = 4;
    config.bigthresh = 40;
    config.reset_on_overflow = false;
    config.maxnodes = 1ULL << 23;

    DMC_decompressor dec(begin, end, config);
    bool lossless = dec.get();
    int lines_pack_log2 = lossless ? 0 : dec.get_symbol(0);
    auto planes = dec.get_symbol(0);
    auto width = dec.get_symbol(0);
    auto height = dec.get_symbol(0);
    std::vector<the_matrix> result;
    result.emplace_back(height,  std::vector<int>(width) );

    if (planes == 2) {
        result.emplace_back(height,  std::vector<int>(width) );
    } else
    if (planes > 2)
    {
        result.emplace_back((height+1)>>1, std::vector<int>((width+1)>>1));
        result.emplace_back((height+1)>>1, std::vector<int>((width+1)>>1));
        if (planes == 4)
            result.emplace_back(height,  std::vector<int>(width) );
    }
    for (auto & plane : result)
    {
        int width = plane[0].size();
        int height = plane.size();
        if (lossless)
        {
            for (int h=0; h<height; h++)
            for (int x=0; x<width; x++)
            {
                const auto left =     x        ? plane[h][x-1]   :0;
                const auto top  =     h        ? plane[h-1][x]   :0;
                const auto topleft =  (x && h) ? plane[h-1][x-1] :0;
                const auto predict = median( top, left, top + left - topleft  );
                plane[h][x] = dec.get_symbol(1) + predict;
            }
        } else {
            int predict = 128;
            int lines_pack = height;
            int height_truncated = height;
            if  ( (1 << lines_pack_log2) < height )
            {
                lines_pack = (1 << lines_pack_log2);
                height_truncated = height & ~(lines_pack-1);
            }

            std::vector<int> predict_lines( lines_pack );

            for (decltype( height) h=0; h<height_truncated; h+=lines_pack)
            {
                #if 0
                int predict_1 = dec.get_symbol(1) + prev_2line;
                int predict_2 = dec.get_symbol(1) + predict_1;
                prev_2line = predict_2;

                plane[h][0] = predict_1;
                plane[h+1][0] = predict_2;
                #endif
                for (int i = 0; i< lines_pack; ++i) {
                    predict = dec.get_symbol(1) + predict;
                    predict_lines[i] = plane[h+i][0] = predict;
                }

                for (decltype( width)  x=1; x<width; x++)
                {
                    #if 0
                    predict_1  = std::clamp( predict_1 + tinyint_to_int( dec.get_symbol(1) ), 0, 255);
                    predict_2  = std::clamp( predict_2 + tinyint_to_int( dec.get_symbol(1) ), 0, 255);
                    plane[h][x] = predict_1;
                    plane[h+1][x] = predict_2;
                    #endif
                    for (int i=0; i<lines_pack;++i)
                    {
                       predict_lines[i] = plane[h+i][x] = std::clamp(  predict_lines[i] + tinyint_to_int( dec.get_symbol(1) ),0,255);
                    }
                }
            }
            if (height_truncated < height)
            {
                lines_pack = height - height_truncated;
                int h = height_truncated;
                for (int i = 0; i< lines_pack; ++i) {
                    predict = dec.get_symbol(1) + predict;
                    predict_lines[i] = plane[h+i][0] = predict;
                }
                for (int x = 1; x < width; x++)
                {
                    for (int i=0; i<lines_pack;++i)
                    {
                       predict_lines[i] = plane[h+i][x] = std::clamp( predict_lines[i] + tinyint_to_int( dec.get_symbol(1)) ,0,255);
                    }
                }
            }
        }
        dec.reset_model();
    }
    return result;
}


int main(int argc, char **argv)
{
    Args args;
    args.parse(argc, argv);
    if (args.size() < 2) {
        std::cout << "usage: \tdmc_img [-d] [-100] [-p<log2_lines>] <image_in> [image_out]\n";
        return 1;
    }
    if (!args.has('d')) {
        auto planes = load_image_as_yuv420(args[1].data());
        auto packed= compress(planes, args.has("100"), args.get('p',1,0,15));
        std::cout << "compressed size : " << packed.size() << std::endl;

        std::ofstream output;
        std::string output_filename =  ( args.size() > 2 ) ? args[2] :  args[1] + ".dmc";
        std::cout << "create " << output_filename << std::endl;
        output.open(output_filename, std::ios::binary | std::ios::out);
        if ( output.fail() ) {
            std::cerr << "can not create output file!!!" << std::endl;
        }
        else {
            output.write(reinterpret_cast<const char *>(packed.data()),packed.size());
        }


    } else {
        std::error_code error;
        mio::ummap_source mmap = mio::make_mmap<mio::ummap_source>(args[1], 0, 0, error);
        if (error)
        {
            std::cout << error.message() << std::endl;
            return 1;
        }

        auto planes = decompress(mmap.cbegin(), mmap.cend());
        std::string output_filename =  ( args.size() > 2 ) ? args[2] :  args[1] + ".bmp";


        store_image_as_yuv420(planes, output_filename.data() );
    }

}
