#include <string>
#include <iomanip>
#include "dmc.hpp"
#include "mio.hpp"
#include <chrono>
#include "myargs/myargs.hpp"
#include <fstream>

int main(int argc, char **argv)
{

    using mio::mmap_source;
    using myargs::Args;
    using pack::Decoder_decompressor;
    using pack::DMCModelConfig;
    using pack::Encoder_compressor;
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;

    Args args;
    args.add_to_group('n', "nodes");
    args.add_to_group('t', "threshold");
    args.add_to_group('T', "threshold-big");
    args.add_to_group('k', "keep-model");
    args.parse(argc, argv);

    DMCModelConfig config;

    config.maxnodes = args.get('n',1ULL<<20, 1ULL, 1ULL << 31);
    config.threshold = args.get('t', 2, 1, 512);
    config.bigthresh = args.get('T', 2, 1, 512);
    config.reset_on_overflow = !args.has('k');

    std::error_code error;
    mio::ummap_source mmap = mio::make_mmap<mio::ummap_source>(args[1], 0, 0, error);
    if (error)
    {
        std::cout << error.message() << std::endl;
        return 1;
    }

    size_t compressed_size = 0;


    bool decode_flag = args.has('d');
    bool bit_planes = args.has('b');
    bool classic = args.has('c');
    if (classic)
        bit_planes = false;

    if (args["profile"] == "fast" ) {
        bit_planes = false;
        classic = true;
        config.maxnodes = 0x8000;
        config.reset_on_overflow = false;
        config.threshold = 4;
        config.bigthresh = 32;
    }

    std::ofstream output;
    if (args.size() >= 3)
    {

        output.open(args[2], std::ios::binary | std::ios::out);
        if (!decode_flag) {
            output.write(reinterpret_cast<const char *>(&config), sizeof(config));
            uint64_t file_size = mmap.size();
            output.write(reinterpret_cast<const char *>(&file_size), sizeof(file_size) );
        }
        classic = true;
        bit_planes = false;
    }

    auto t1 = high_resolution_clock::now();
    if (decode_flag)
    {
        Reader reader(mmap.begin(), mmap.end());
        auto config = reader.read<DMCModelConfig>();
        auto file_size = *reader.read<uint64_t>();

        std::cout << "hdr:" << std::endl;
        std::cout << "\tfile-size: " << file_size << std::endl;
        std::cout << "\tthreshold: " << config->threshold << std::endl;
        std::cout << "\tbig thresh: " << config->bigthresh << std::endl;
        std::cout << "\twhen model overflow: " << (config->reset_on_overflow ? "reset" : "keep") << std::endl;
        auto t1 = high_resolution_clock::now();
        auto start_pos = reader.begin();
        Decoder_decompressor dmc(reader, *config);
        std::vector<uint8_t> buffer;

        for (uint64_t pos = 0; pos < file_size; ++pos) {
            buffer.push_back(  dmc.get_symbol_bits<8>() );
            if ( buffer.size() >=  0x100000 || pos == file_size-1)  {
                    auto cur_pos = dmc.position() -  start_pos;
                    auto s = duration<double>(high_resolution_clock::now() - t1).count();
                    auto speed_mbs = static_cast<uint64_t>(pos / s) >> 20;
                    if (s < 0.0000001)
                        s = 0.0000001;
                    output.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
                    buffer.clear();
                    std::cout << "\r" << (cur_pos >> 20) << " -> " << (pos >> 20) << " MB, speed (avg): " << speed_mbs << "MB/s, percent: " << ((pos+1)*100 / file_size) << "%" << "        \b\b\b\b\b\b\b\b" << std::flush;
            }
        }
        std::cout << std::endl;
    }
    else
    {
        Encoder_compressor dmc(config);
        for (int bit_plane = 7; bit_plane >= 0; bit_plane--)
        {
            size_t pos = 0;
            size_t old_pos = 0;

            for (auto symbol : mmap)
            {

                if (classic)
                {
                    dmc.put_symbol_bits<8>(symbol);
                }
                else
                {
                    if (bit_planes)
                        dmc.put(symbol & (1 << bit_plane));
                    else
                        dmc.put_symbol(symbol, 0);
                }
                pos++;
                if (dmc.get_bytes_count() > 0x100000)
                {
                    auto s = duration<double>(high_resolution_clock::now() - t1).count();
                    auto part = pos - old_pos;
                    old_pos = pos;
                    if (bit_planes)
                        part >>= 8;
                    // skip wierd case
                    auto part_packed = dmc.get_bytes();
                    compressed_size += part_packed.size();

                    if (s > 0 && part > 0 && part_packed.size() > 0)
                    {
                        auto speed_mbs = static_cast<size_t>(part / s) >> 20;
                        std::cout << "\r" << (pos >> 20) << " -> " << (compressed_size >> 20) << " MB, speed: " << speed_mbs << " MB/s, ratio: " << std::fixed << std::setprecision(3) << ( static_cast<float>(part) / part_packed.size()) << ":1, nodes: " << dmc.get_nodes_count() << "        \b\b\b\b\b\b\b\b" << std::flush;
                    }
                    if (output.is_open() && part_packed.size())
                    {
                        output.write(reinterpret_cast<const char *>(part_packed.data()), part_packed.size());
                    }
                    t1 = high_resolution_clock::now();
                }
            }
            // std::cout << std::endl;
            if (!bit_planes)
                break;
            dmc.reset_model();
        }
        auto last_part = dmc.finish();
        if (output.is_open() && last_part.size()) {
            output.write(reinterpret_cast<const char *>(last_part.data()), last_part.size());
        }
        compressed_size += last_part.size();
        std::cout << "\rcompressed " << mmap.size() << " -> " << compressed_size << " bytes (" << (compressed_size >> 20) << " MB),  nodes: " << dmc.get_nodes_count() << "        \b\b\b\b\b\b\b\b" << std::endl
                  << std::endl;
    }
}
