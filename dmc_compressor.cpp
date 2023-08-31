#include <string>
#include "dmc.hpp"
#include "mio.hpp"
#include "myargs/myargs.hpp"

int main(int argc, char ** argv) {

    using pack::DMC_compressor;
    using mio::mmap_source;
    using myargs::Args;

    Args args;
    args.add_to_group('n',"nodes");
    args.add_to_group('t',"threshold");
    args.add_to_group('T',"threshold-big");
    args.add_to_group('1',"single-state");
    args.add_to_group('k',"keep");
    args.parse(argc,argv);

    DMC_compressor dmc;

    dmc.reset_model( args.get('n',1<<20, 1,1<<30), args.get('t',2,1,512), args.get('T',2,1,512), !args.has('k') );
    std::error_code error;
    mio::mmap_source mmap = mio::make_mmap_source(argv[1], error);
    if (error)  {
        std::cout << error.message() << std::endl;
        return 1;
    }
    auto single_state  = args.has('1');
    size_t compressed_size = 0;

    size_t pos = 0;
    for (auto symbol :  mmap) {
        if (single_state)
            dmc.put_symbol_bits_single<8>(symbol);
        else
            dmc.put_symbol_bits<8>(symbol);
        pos++;
        if (dmc.get_bytes_count() > 0x100000) {
            compressed_size += dmc.get_bytes().size();
            std::cout << "\r                       \r" << (pos >> 20) << " -> " << (compressed_size >> 20) <<  " Mb,  nodes: " << dmc.get_nodes_count() << std::flush;
        }
    }
    compressed_size += dmc.finish().size();
    std::cout << "\r                                    \rcompressed " << compressed_size << " bytes (" << (compressed_size >> 20) << " Mb),  nodes: " << dmc.get_nodes_count() << std::endl << std::endl;

}
