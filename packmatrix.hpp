#pragma once
#include <vector>
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <utility>
#include "the_matrix.hpp"
#include "huffman.hpp"
#include "utils.hpp"
#include "dmc.hpp"
#include "cabacH265.hpp"
#include "rangecoder.hpp"
#include "bitstream_helper.hpp"

namespace pack {


namespace huffman {

inline std::vector<uint8_t> compress(const the_matrix& matrix) {
    auto data = flatten(matrix);
    //std::cerr << "flatten matrix: " << data << std::endl;
    Huffman<int> henc;
    BitWriter enc_w;
    enc_w.set_ur_golomb(matrix[0].size(),8,16,32);
    enc_w.set_ur_golomb(matrix.size(),8,16,32 );
    henc.buildHuffmanTree(data.begin(), data.end());
    henc.encodeHuffmanTree(enc_w);
    //std::cerr << "huffman table size = " << enc_w.size() << std::endl;
    henc.encode(enc_w,data.begin(),data.end());
    return enc_w.get_all_bytes();
}


inline the_matrix decompress(const std::vector<uint8_t>& compressed) {
    Huffman<int> hdec;
    BitReader enc_r(compressed.data(),compressed.size()*8);
    auto width = enc_r.get_ur_golomb(8,16,32);
    auto height = enc_r.get_ur_golomb(8,16,32);

    hdec.decodeHuffmanTree(enc_r);

    the_matrix matrix(height, std::vector<int>(width));
    for (auto & row : matrix)
    for (auto & val : row)
        val = hdec.decode(enc_r);
    return matrix;
}
}

namespace CABAC {
inline std::vector<uint8_t> compress(const the_matrix& matrix) {

    H265_compressor enc;

    enc.put_symbol(matrix[0].size(), 0); //width
    enc.put_symbol(matrix.size(), 0);  //height

    for (auto & row : matrix)
    for (auto & val : row)
        enc.put_symbol(val,1);


    return enc.finish();
}

inline the_matrix decompress(const std::vector<uint8_t>& compressed) {

    H265_decompressor dec(compressed.begin(), compressed.end());

    auto width = dec.get_symbol(0);
    auto height = dec.get_symbol(0);
    the_matrix matrix(height, std::vector<int>(width));
    for (auto & row : matrix)
    for (auto & val : row)
        val = dec.get_symbol(1);

    return matrix;

}
}

namespace DMC {

inline std::vector<uint8_t> compress(const the_matrix& matrix) {

    DMCModelConfig config;
    config.threshold = 4;
    config.bigthresh = 40;
    config.reset_on_overflow = false;
    config.maxnodes = 65536;
    DMC_compressor enc(config);

    enc.put_symbol(matrix[0].size(), 0); //width
    enc.put_symbol(matrix.size(), 0);  //height

    for (auto & row : matrix)
    for (auto & val : row)
        enc.put_symbol(val,1);

    return enc.finish();
}

inline the_matrix decompress(const std::vector<uint8_t>& compressed) {

    DMCModelConfig config;
    config.threshold = 4;
    config.bigthresh = 40;
    config.reset_on_overflow = false;
    config.maxnodes = 65536;

    DMC_decompressor dec(compressed.begin(), compressed.end(), config);

    auto width = dec.get_symbol(0);
    auto height = dec.get_symbol(0);
    the_matrix matrix(height, std::vector<int>(width));
    for (auto & row : matrix)
    for (auto & val : row)
        val = dec.get_symbol(1);

    return matrix;

}
} //namespace dmc

namespace rangecoder {

inline std::vector<uint8_t> compress(const the_matrix& matrix) {

    RangeCoder_compressor enc;

    enc.put_symbol(matrix[0].size(), 0); //width
    enc.put_symbol(matrix.size(), 0);  //height

    for (auto & row : matrix)
    for (auto & val : row)
        enc.put_symbol(val,1);


    return enc.finish();
}

inline the_matrix decompress(const std::vector<uint8_t>& compressed) {


    RangeCoder_decompressor dec(compressed.begin(), compressed.end());

    auto width = dec.get_symbol(0);
    auto height = dec.get_symbol(0);
    the_matrix matrix(height, std::vector<int>(width));
    for (auto & row : matrix)
    for (auto & val : row)
        val = dec.get_symbol(1);
    return matrix;
}

}

}  // namespace pack