#pragma once

class ValueWriter {

inline int ilog2_32(uint32_t v)
{
   if (v == 0)
      return 1;
   return 32-__builtin_clz(v);
}

    public:
    ValueWriter() = delete;
    ValueWriter(BitWriter &ref) : ref(ref) {};
    void encode( int n, int value)
    {
        auto base = (1 << n)-1;
        if (value >= base)
        {
            value -= base;
            auto n_ = std::max(n, ilog2_32(value));
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
    private:
    BitReader& ref;
};
