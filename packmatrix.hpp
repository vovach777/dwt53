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
#include "bitstream_helper.hpp"

namespace pack {



    template <typename Base>
    class Compressor {
        using State = typename Base::StateType;
        public:
            Compressor(Base & encoder) : encoder(encoder), stateBitSize(Base::defaultState()), stateValue(Base::defaultState()) {};
            inline void bit(bool b,State &state) {
                encoder.put(b, state);
            }
            inline void bit_bypass(bool b) {
                encoder.put_bypass(b);
            }

            void bits(int n, int value, State &state) {
                if (ilog2_32(value,1) > n)
                    throw std::logic_error("value not fit in n-bits!");

                for (unsigned mask = 1 << (n-1); mask; mask >>= 1) {

                    bit(value & mask, state);
                }
            }
            void bits_bypass(int n, int value) {
                if (ilog2_32(value,1) > n)
                    throw std::logic_error("value not fit in n-bits!");

                for (unsigned mask = 1 << (n-1); mask; mask >>= 1) {

                    bit_bypass(value & mask);
                }
            }

            void unary_jpair(int value) {
                // value = to_jpair(value);
                // const auto bitlen = value & 0xf;
                // for (int unary=bitlen;  unary; --unary) {
                //     bit(1,stateBitSize);
                // }
                // bit(0,stateBitSize);
                // if (bitlen)
                //    bits(bitlen, value >> 4,stateValue);
                auto val = s2u(value);
                while (val--)
                    bit_bypass(1);
                 bit_bypass(0);

            }
            auto finish() {
                return encoder.finish();
            }
            private:
                Base& encoder;
                State stateBitSize;
                State stateValue;
    };


    template<typename Base>
    class Decompressor {
        using State = typename Base::StateType;
        public:
        Decompressor(Base & decoder) : decoder(decoder), stateBitSize(Base::defaultState()), stateValue(Base::defaultState()) {};

        inline auto bit_bypass() {
            return decoder.get_bypass();
        }

        inline auto bit(State& state) {
            return decoder.get(state);
        }

        int bits_bypass(int n) {
            if (n < 1)
                throw std::logic_error("can not read less than 1-bit!");
            int value=0;
            while (n--) {
                auto bit = bit_bypass();
                if (bit > 1)
                    throw std::logic_error("special symbol detected!");
                value = value + value + bit_bypass();
            }
            return value;
        }

        int bits(int n, State& state) {
            if (n < 1)
                throw std::logic_error("can not read less than 1-bit!");
            int value=0;
            while (n--) {
                value = value + value + bit(state);
            }
            return value;
        }
        int unary_jpair() {
            // int bitlen = 0;
            // while (bit(stateBitSize))
            //     bitlen++;
            // if (bitlen == 0)
            //     return 0;
            // return from_jpair( make_jpair( bitlen, bits(bitlen, stateValue) ) );
            unsigned val = 0;
            while (decoder.get_bypass())
                val++;
            return u2s(val);
        }
        private:
            Base& decoder;
            State stateBitSize;
            State stateValue;
    };


namespace huffman {

inline std::vector<uint8_t> compress(const the_matrix& matrix) {
    auto data = flatten(matrix);
    std::cerr << "flatten matrix: " << data << std::endl;
    Huffman<int> henc;
    BitWriter enc_w;
    enc_w.writeBits(16, matrix[0].size());
    enc_w.writeBits(16, matrix.size());
    henc.buildHuffmanTree(data.begin(), data.end());
    henc.encodeHuffmanTree(enc_w);


    //std::cerr << "huffman table size = " << enc_w.size() << std::endl;

    henc.encode(enc_w,data.begin(),data.end());
    return enc_w.get_all_bytes();
}



inline the_matrix decompress(const std::vector<uint8_t>& compressed) {
    Huffman<int> hdec;
    BitReader enc_r(compressed.data(),compressed.size());
    auto width = enc_r.readBits(16);
    auto height = enc_r.readBits(16);
    hdec.decodeHuffmanTree(enc_r);
    return make_matrix( width, height,
    [&](int &v) {
        v = hdec.decode(enc_r);
    }
    );
}
}

namespace CABAC {
inline std::vector<uint8_t> compress(const the_matrix& matrix) {

    std::vector<uint8_t> result; //NRVO please
    auto cabac_codec = H265_compressor(result);
    auto enc = Compressor( cabac_codec );

    // enc.bits_bypass(16,matrix[0].size());
    // enc.bits_bypass(16,matrix.size());
    enc.unary_jpair(matrix[0].size());
    enc.unary_jpair(matrix.size());
    process_matrix(matrix, [&](int v){
        enc.unary_jpair(v);
    });
    enc.finish();
    std::cout << result << std::endl;
    return result;
}

inline the_matrix decompress(const std::vector<uint8_t>& compressed) {

    auto cabac_codec =  H265_decompressor( compressed.begin(),compressed.end());
    auto dec = Decompressor( cabac_codec );
    auto width = dec.unary_jpair();
    auto height = dec.unary_jpair();
    return make_matrix( width, height,
    [&](int &v) {
        v = dec.unary_jpair();
    }
    );
}
}

namespace DMC {

inline std::vector<uint8_t> compress(const the_matrix& matrix) {

    auto dmc_codec = DMC_compressor();
    auto enc = Compressor(dmc_codec);
    enc.unary_jpair(matrix[0].size());
    enc.unary_jpair(matrix.size());
    process_matrix(matrix, [&](int v){
        enc.unary_jpair(v);

    });
    return enc.finish();
}

inline the_matrix decompress(const std::vector<uint8_t>& compressed) {
    auto dmc_codec = DMC_decompressor(compressed.data(), compressed.size()*8);
    auto dec = Decompressor(dmc_codec);
    auto width = dec.unary_jpair();
    auto height = dec.unary_jpair();
    return make_matrix( width, height,
    [&](int &v) {
        v = dec.unary_jpair();
    }
    );
}

}


}  // namespace pack