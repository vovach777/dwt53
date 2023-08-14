
#include <unordered_map>
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <vector>
#include "huffman.hpp"
#include "packmatrix.hpp"
namespace huffman {

std::vector<uint8_t> compress(const the_matrix& matrix) {
    auto data = flatten(matrix);
    compress::Huffman<int> henc;
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



the_matrix decompress(const std::vector<uint8_t>& compressed) {
    compress::Huffman<int> hdec;
    BitReader enc_r(compressed.data(),compressed.size());
    hdec.decodeHuffmanTree(enc_r);   
    return make_matrix( enc_r.readBits(16), enc_r.readBits(16),
    [&](int &v) {
        v = hdec.decode(enc_r);
    }
    );
}

};  // namespace huffman