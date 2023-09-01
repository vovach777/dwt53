#include <string>
#include "dmc.hpp"
#include "mio.hpp"
#include <chrono>
#include "myargs/myargs.hpp"

int main(int argc, char ** argv) {

    using pack::Encoder_compressor;
    using pack::DMCModelConfig;
    using mio::mmap_source;
    using myargs::Args;
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;


    Args args;
    args.add_to_group('n',"nodes");
    args.add_to_group('t',"threshold");
    args.add_to_group('T',"threshold-big");
    args.add_to_group('k',"keep-model");
    args.parse(argc,argv);

    DMCModelConfig config;

    config.maxnodes = args.get('n',1<<20, 1,1<<30);
    config.threshold = args.get('t',2,1,512);
    config.bigthresh = args.get('T',2,1,512);
    config.reset_on_overflow = !args.has('k');
    Encoder_compressor dmc(config);

    std::error_code error;
    mio::mmap_source mmap = mio::make_mmap_source(args[1], error);
    if (error)  {
        std::cout << error.message() << std::endl;
        return 1;
    }

    size_t compressed_size = 0;

    size_t pos = 0;
    size_t old_pos = 0;
    auto t1 = high_resolution_clock::now();
    for (auto symbol :  mmap) {


        dmc.put_symbol_bits<8>(symbol);
        pos++;
        if (dmc.get_bytes_count() > 0x100000) {
            auto part = pos - old_pos;
            old_pos = pos;
            auto s = duration<double>(high_resolution_clock::now() - t1).count();
            //skip wierd case
            auto part_packed = dmc.get_bytes().size();
            compressed_size += part_packed;

            if (s > 0 && part > 0 && part_packed > 0 )
            {
                auto speed_mbs = static_cast<size_t>(part / s) >> 20;
                std::cout << "\r"  << (pos >> 20) << " -> " << (compressed_size >> 20) <<  " MB, speed: " << speed_mbs  << " MB/s,  ratio: " << (part / part_packed)  << ":1 , nodes: " << dmc.get_nodes_count() << "        \b\b\b\b\b\b\b\b" << std::flush;
            }
            t1 = high_resolution_clock::now();
        }
    }
    compressed_size += dmc.finish().size();
    std::cout << "\rcompressed " << compressed_size << " bytes (" << (compressed_size >> 20) << " MB),  nodes: " << dmc.get_nodes_count() << "        \b\b\b\b\b\b\b\b" << std::endl << std::endl;

}
