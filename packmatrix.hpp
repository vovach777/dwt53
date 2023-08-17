#pragma once
#include <vector>
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <vector>
#include "the_matrix.hpp"
#include "huffman.hpp"
#include "utils.hpp"
#include "dmc.hpp"
#include "bitstream_helper.hpp"

namespace pack {

namespace huffman {

inline std::vector<uint8_t> compress(const the_matrix& matrix) {
    auto data = flatten(matrix);
    Huffman<int> henc;
    BitWriter enc_w;
    henc.buildHuffmanTree(data.begin(), data.end());
    henc.encodeHuffmanTree(enc_w);
        
    std::cerr << "haffmantable size = " << enc_w.size() << std::endl;

    enc_w.writeBits(16, matrix[0].size());
    enc_w.writeBits(16, matrix.size());    
    henc.encode(enc_w,data.begin(),data.end());
    enc_w.flush();

    return std::vector<uint8_t>(enc_w.data(), enc_w.data() + enc_w.size());
}



inline the_matrix decompress(const std::vector<uint8_t>& compressed) {
    Huffman<int> hdec;
    BitReader enc_r(compressed.data(),compressed.size());
    hdec.decodeHuffmanTree(enc_r);   
    auto width = enc_r.readBits(16);
    auto height = enc_r.readBits(16);
    return make_matrix( width, height,
    [&](int &v) {
        v = hdec.decode(enc_r);
    }
    );
}
}

namespace DMC {
inline std::vector<uint8_t> compress(const the_matrix& matrix) {
    dmc enc;
    BitWriter bw;
    ValueWriter vw(bw);
    vw.encode_golomb(8,matrix[0].size());
    vw.encode_golomb(8,matrix.size());
    int k = 8;
    process_matrix(matrix, [&](int v){
        vw.encode_golomb(k,s2u(v));
        k = (k + k + ilog2_32(s2u(v),0)) / 3;
    });
    bw.flush();
    BitReader br(bw.data(),bw.size(), bw.size_in_bits());
    while (br.bit_left()) {
        enc.comp(br.readBit());
    }
    enc.comp_eof();
    return enc.get_encoded();
}

inline the_matrix decompress(const std::vector<uint8_t>& compressed) {
    dmc dec;
    auto it = compressed.begin();
    BitWriter bw;
    for(;;) {
        auto bit = dec.exp(it,compressed.begin(), compressed.end());
        if (bit < 0) {
            break;
        }
        bw.writeBit(bit);
    }
    bw.flush();
    BitReader br(bw.data(),bw.size(), bw.size_in_bits());
    ValueReader vr(br);
    auto width = vr.decode_golomb(8);
    auto height = vr.decode_golomb(8);
    int k = 8;
    return make_matrix( width, height,
    [&](int &v) {
        if (br.bit_left() == 0)
            {
                v = 0;
                return;
            }
        auto uv = vr.decode_golomb(k);
        k = (k + k + ilog2_32(uv,0)) / 3;
        v = u2s(uv);
    }
    );
}
}

}  // namespace pack