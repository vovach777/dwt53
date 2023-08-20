#pragma once
#include <vector>
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include "the_matrix.hpp"
#include "huffman.hpp"
#include "utils.hpp"
#include "cabac265.hpp"
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

namespace CABAC {
   
    template<typename W>
    class Compressor : public H265Writer<W> {
        public:
            Compressor(W& w) : H265Writer<W>(w) {};
            inline void bit(int b,  uint8_t &state, bool bypass=false) {
                if (bypass)
                    H265Writer<W>::put_bypass(b);
                else
                    H265Writer<W>::put(b, state);
            }
            void bits(int n, int value, uint8_t &state, bool bypass=false) {
                if (ilog2_32(value,1) > n)
                    throw std::logic_error("value not fit in n-bits!");
                
                for (unsigned mask = 1 << (n-1); mask; mask >>= 1) {

                    bit(value & mask, state,bypass);
                }
            }
            void unary_jpair(int value) {
                value = to_jpair(value);
                const auto bitlen = value & 0xf;
                for (int unary=bitlen;  unary; --unary) {
                    bit(1,states[0]);
                }
                bit(0,states[0]);
                if (bitlen)
                   bits(bitlen, value >> 4,states[1]);
            }
            private:
                uint8_t states[2] = {0,0};

    };
    template <typename R>
    class Decompressor : public H265Reader<R> {
        public:
        Decompressor(R& r) : H265Reader<R>(r) {};
        inline int bit(uint8_t& state, bool bypass = false) {      
            return bypass ? H265Reader<R>::get_bypass() : H265Reader<R>::get(state);
        }
        int bits(int n, uint8_t& state, bool bypass=false) {
            if (n < 1) 
                throw std::logic_error("can not read less than 1-bit!");
            int value=0;
            while (n--) {
                value = value + value + bit(state, bypass);
            }
            return value;
        }
        int unary_jpair() {
            int bitlen = 0;
            while (bit(states[0])) 
                bitlen++;
            if (bitlen == 0)
                return 0;
            return from_jpair( make_jpair( bitlen, bits(bitlen, states[1]) ) );
        }
        private:
        uint8_t states[2] = {0,0};
    };

    inline std::vector<uint8_t> compress(const the_matrix& matrix) {
        WriterVec vec;
        Compressor enc(vec);

 
        enc.unary_jpair(matrix[0].size());
        enc.unary_jpair(matrix.size());
    
        process_matrix(matrix, [&](int v){
            enc.unary_jpair(v);
        });
    
        enc.finish();
 
        return std::move(vec.get());
    }

inline the_matrix decompress(const std::vector<uint8_t>& compressed) {
    ReaderIt vec(compressed.begin(), compressed.end());
    Decompressor dec(vec);
    auto width = dec.unary_jpair();
    auto height = dec.unary_jpair();
    return make_matrix(width, height, [&](int&v) {
        v = dec.unary_jpair();
    });
}

}




}  // namespace pack