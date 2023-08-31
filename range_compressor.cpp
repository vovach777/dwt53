#include <string>
#include "rangecoder.hpp"
#include "mio.hpp"

int main(int argc, char ** argv) {

    using pack::RangeCoder_compressor;
    using mio::mmap_source;

    RangeCoder_compressor rac;
    std::error_code error;
    mio::mmap_source mmap = mio::make_mmap_source(argv[1], error);
    if (error)  {
        std::cout << error.message() << std::endl;
        return 1;
    }
    size_t compressed_size = 0;
    for (auto symbol :  mmap) {
        rac.put_symbol(symbol,0);
        if (rac.get_bytes_count() > 0x10000) {
            compressed_size += rac.get_bytes().size();
            std::cout << "\r      \rcompression " << compressed_size << std::flush;
        }
    }
    auto bytes = rac.finish();
    compressed_size += bytes.size();
            std::cout << "\r      \rcompressed " << compressed_size << std::endl;

}