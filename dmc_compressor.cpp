#include <string>
#include "dmc.hpp"
#include "mio.hpp"

int main(int argc, char ** argv) {

    using pack::DMC_compressor;
    using mio::mmap_source;

    DMC_compressor dmc;
    dmc.reset_model(64000000);
    std::error_code error;
    mio::mmap_source mmap = mio::make_mmap_source(argv[1], error);
    if (error)  {
        std::cout << error.message() << std::endl;
        return 1;
    }
    for (auto symbol :  mmap) {
        dmc.put_symbol(symbol,0);
    }
    auto bytes = dmc.finish();
    std::cout << "compressed size: " << bytes.size() << " nodes: " << dmc.get_nodes_count() << std::endl;

}