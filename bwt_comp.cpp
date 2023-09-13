#include <string>
#include <iomanip>
#include "dmc.hpp"
#include "mio.hpp"
#include <chrono>
#include "myargs/myargs.hpp"
#include <fstream>
#include <utility>
#include <algorithm>
#include "utils.hpp"
#include "bbwt64.hpp"
#include <bitset>


#ifdef _WIN32
auto phys_mem() {
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);

    GlobalMemoryStatusEx(&status);

    return status.ullTotalPhys;

}
#else
#if __has_include(<unistd.h>)
#include <unistd.h>
    auto phys_mem() {
        auto pageSize = sysconf(_SC_PAGESIZE);
        auto physPages = sysconf(_SC_PHYS_PAGES);
        return static_cast<uint64_t>(pageSize) * physPages;
    }
#else
    auto phys_mem() {
        return 640*1024;
    }
#endif
#endif

uint64_t mtf64(uint64_t value) {
    std::bitset<64> v64{value};
    bool front = false;
    for (int pos=63; pos >= 0; --pos) {
        if ( v64[pos] == front)
            v64[pos] = false;
        else {
            v64[pos] = true;
            front = !front; //move-to-front
        }
    }
}

int main(int argc, char **argv)
{

    using mio::mmap_source;
    using myargs::Args;
    using pack::DMC_decompressor;
    using pack::DMCModelConfig;
    using pack::DMC_compressor;
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;

    Args args;
    args.parse(argc, argv);


    std::error_code error;
    mio::ummap_source mmap = mio::make_mmap<mio::ummap_source>(args[1], 0, 0, error);
    if (error)
    {
        std::cout << error.message() << std::endl;
        return 1;
    }
    const  uint64_t* map64 = reinterpret_cast<const uint64_t*>(&mmap[0]);
    uint64_t map64_size = mmap.size() / 8;

    size_t compressed_size = 0;


    bool decode_flag = args.has('d');

    std::ofstream output;
    if (args.size() >= 3)
    {
        output.open(args[2], std::ios::binary | std::ios::out);
    }

    auto t1 = high_resolution_clock::now();

    size_t pos = 0;
    size_t old_pos = 0;

    auto map64_p = map64;
    for (auto nb = map64_size; nb; --nb)
    {

        auto [val64, inx] = bwt_encode(*map64_p++);



    }
// for (auto begin = reinterpret_cast<const uint8_t*>(map64_p), end = begin +( mmap.size() & 7); begin != end; ++begin )
//     dmc.put_symbol_bits<8>(*begin);


}
