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
#include "quantization.hpp"

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

namespace MTF {
    inline std::vector<uint8_t> compress(const the_matrix& matrix) {
        DMCModelConfig config;
        config.threshold = 4;
        config.bigthresh = 40;
        config.reset_on_overflow = false;
        config.maxnodes = 65536;
        DMC_compressor enc(config);
        auto range = get_range(matrix);
        std::vector<int> mtf;
        for (int i = range.min; i <= range.max; ++i) {
             mtf.push_back(i);
        }
        enc.put_symbol(matrix[0].size(), 0); //width
        enc.put_symbol(matrix.size(), 0);  //height
        enc.put_symbol(range.min,1);
        enc.put_symbol(range.max,1);

        // enc.put_symbol(mtf.size(), 0);  //codebook size
        // //codebook values
        // auto scan_mtf = mtf.begin();
        // int prev = *scan_mtf++;
        // enc.put_symbol(prev,1);
        // //enc.reset_model();

        // while ( scan_mtf != mtf.end() ) {
        //     enc.put_symbol(*scan_mtf - prev,0);
        //     prev = *scan_mtf++;
        // }

        for (auto & row : matrix)
        for (auto & val : row) {
            //auto qval = quantizer[val];
            auto it = std::find(mtf.begin(), mtf.end(), val); //find index
            if ( it == mtf.end() ) { //if not such symbol
                enc.put_symbol(0,0); // replace with prev symbol
            } else {
                enc.put_symbol(std::distance(mtf.begin(), it) ,0); //send index
                if ( it != mtf.begin()) //update front
                    std::rotate(mtf.begin(),it,it+1);
            }
        }

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
    auto range_min = dec.get_symbol(1);
    auto range_max = dec.get_symbol(1);
    std::vector<int> mtf;
    for (int i = range_min; i<= range_max; ++i)
        mtf.push_back(i);

    // std::vector<int> mtf( dec.get_symbol(0) );
    // int prev = mtf[0] = dec.get_symbol(1);
    // for (int i=1; i<mtf.size(); ++i) {
    //     prev = mtf[i] = dec.get_symbol(0)  + prev;
    // }


    for (auto & row : matrix)
    for (auto & val : row) {
        int index = dec.get_symbol(0);
        val = mtf.at( index );
        auto it = mtf.begin() + index;
        if ( it != mtf.begin()) //update front
            std::rotate(mtf.begin(),it,it+1);

    }

    return matrix;

}

}


inline uint64_t ror1_ip(uint64_t &x)
{
   return (x >> 1) | (x << 63);
}


uint64_t bwt_encode(uint64_t value) {

    std::vector<uint64_t> v64(64);

    for (auto &v : v64)
        v = value = ror1_ip(value);
    std::stable_sort(v64.begin(), v64.end());

    uint64_t res = 0;
    for (auto &v : v64)
        res = res + res + (v&1);
    return res;
}

template <typename ReadBit>
uint64_t bwt_decode(uint64_t value) {

    std::vector<uint64_t> v64(64, 0);

    for (int pass=0; pass < 64; pass++)
    {
        for (int v = 63; v >= 0; --v)
            v64[63-v] |= ((value >> v) & 1) << pass;
        std::stable_sort(v64.begin(), v64.end());
    }

    int res = 0;
    for (auto &v : v64)
        res = res + res + (v&1);
    return res;

}


}  // namespace pack