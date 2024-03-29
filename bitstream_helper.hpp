#pragma once
#include "bitstream.hpp"




class ValueWriter {
    public:
    ValueWriter() = delete;
    ValueWriter(BitWriter &ref) : ref(ref) {};
    ValueWriter(const ValueWriter & other): ref(other.ref) {};

    ValueWriter& operator=(const ValueWriter& other) {
        ref = other.ref;
        return *this;
    }

    void encode( int n, int value)
    {
        auto base = (1 << n)-1;
        if (value >= base)
        {
            value -= base;
            auto n_ = std::max DUMMY (n, ilog2_32(value));
            base = (1 << n_)-1;
            n = n_+1+n_;
            if (n > 32) {
                ref.writeBits(n_,base);
                ref.writeBits(1, 0);
                ref.writeBits(n_, value);
                return;
            }
            value = (base << (n_+1)) | value;
        }
        ref.writeBits(n,value);
    }

    inline void encode_unary( int value, bool is_signed) {
        if (is_signed) {
            value = s2u(value);
        }
        while (value > 32 ) {
            ref.writeBits(32,0xffffffff);
            value -= 32;
        }
        while (value > 8 ) {
            ref.writeBits(8,0xff);
            value -= 8;
        }
        while (value > 0 ) {
            ref.writeBit(true);
            value -= 1;
        }
        ref.writeBit(false);

    }

    void encode_golomb( unsigned k, unsigned value ) {

        auto n_ = ilog2_32(value>>k,0);
        auto base = (1 << n_)-1;
        auto n = n_+1+k+n_;
        if (n > 32) {
            ref.writeBits(n_,base);
            ref.writeBits(1, 0);
            ref.writeBits(n_+k, value);
            return;
        }
        value = (base << (n_+1+k)) | value;
        ref.writeBits(n,value);
    }


    private:
    BitWriter&ref;
};

class ValueReader {
    public:
    ValueReader() = delete;
    ValueReader(BitReader& ref) : ref(ref) {}
    int decode( int n )
    {

        auto v = ref.readBits(n);
        if (v == ((1 << n)-1)) {

            while (ref.readBit()) ++n;
            v += ref.readBits(n);
        }

        return v;
    }
unsigned decode_golomb(unsigned k) {

    while (ref.readBit()) ++k;
    return ref.readBits(k);
}
    private:
    BitReader& ref;
};
