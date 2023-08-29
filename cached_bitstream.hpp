#pragma once
#include <cstdint>
#include <climits>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include "utils.hpp"

namespace ffmpeg {



struct BitstreamContextBE {
    uint64_t bits = 0; // stores bits read from the buffer
    const uint8_t *buffer = nullptr, *buffer_end = nullptr;
    const uint8_t *ptr = nullptr; // pointer to the position inside a buffer
    int bits_valid = 0; // number of bits left in bits field
    int64_t size_in_bits = 0;

/**

 * @return

 * - 0 on successful refill

 * - a negative number when bitstream end is hit

 *

 * Always succeeds when UNCHECKED_BITSTREAM_READER is enabled.

 */
inline int bits_priv_refill_64_be()
{
    if (ptr >= buffer_end)
        return -1;
    bits = av_bswap64((((const union unaligned_64 *) (ptr))->l));
    ptr += 8;
    bits_valid = 64;
    return 0;
}
/**

 * @return

 * - 0 on successful refill

 * - a negative number when bitstream end is hit

 *

 * Always succeeds when UNCHECKED_BITSTREAM_READER is enabled.

 */
inline int bits_priv_refill_32_be()
{
    if (ptr >= buffer_end)
        return -1;
    bits |= (uint64_t)av_bswap32((((const union unaligned_32 *) (ptr))->l)) << (32 - bits_valid);
    ptr += 4;
    bits_valid += 32;
    return 0;
}
/**

 * Initialize BitstreamContext.

 * @param buffer bitstream buffer, must be AV_INPUT_BUFFER_PADDING_SIZE bytes

 *        larger than the actual read bits because some optimized bitstream

 *        readers read 32 or 64 bits at once and could read over the end

 * @param bit_size the size of the buffer in bits

 * @return 0 on success, AVERROR_INVALIDDATA if the buffer_size would overflow.

 */
BitstreamContextBE() = default;


BitstreamContextBE(const uint8_t *buffer,  int64_t bit_size)
{
    size_t buffer_size;
    buffer_size = (bit_size + 7) >> 3;
    this->buffer = buffer;
    buffer_end = buffer + buffer_size;
    ptr = buffer;
    size_in_bits = bit_size;
    bits_valid = 0;
    bits = 0;

    if (buffer_end < ptr || buffer == nullptr) {
        throw std::length_error("Invalid data found when processing input");
    }

    bits_priv_refill_64_be();
}
/**

 * Return number of bits already read.

 */
inline auto bits_tell_be()
{
    return (ptr - buffer) * 8 - bits_valid;
}
/**

 * Return buffer size in bits.

 */
inline auto bits_size_be()
{
    return size_in_bits;
}
/**

 * Return the number of the bits left in a buffer.

 */
inline auto bits_left_be()
{
    return (buffer - ptr) * 8 + size_in_bits + bits_valid;
}

inline uint64_t bits_priv_val_show_be(unsigned int n)
{
    return bits >> (64 - n);
}
inline void bits_priv_skip_remaining_be(unsigned int n)
{
    bits <<= n;
    bits_valid -= n;
}
inline uint64_t bits_priv_val_get_be(unsigned int n)
{
    uint64_t ret;
    ret = bits_priv_val_show_be(n);
    bits_priv_skip_remaining_be(n);
    return ret;
}
/**

 * Return one bit from the buffer.

 */
inline unsigned int bits_read_bit_be()
{
    if (!bits_valid && bits_priv_refill_64_be() < 0)
        return 0;
    return bits_priv_val_get_be(1);
}
/**

 * Return n bits from the buffer, n has to be in the 1-32 range.

 * May be faster than bits_read() when n is not a compile-time constant and is

 * known to be non-zero;

 */
inline uint32_t bits_read_nz_be(unsigned int n)
{
    if (n > bits_valid) {
        if (bits_priv_refill_32_be() < 0)
            bits_valid = n;
    }
    return bits_priv_val_get_be(n);
}
/**

 * Return n bits from the buffer, n has to be in the 0-32  range.

 */
inline uint32_t bits_read_be(unsigned int n)
{
    if (!n)
        return 0;
    return bits_read_nz_be(n);
}
/**

 * Return n bits from the buffer, n has to be in the 0-63 range.

 */
inline uint64_t bits_read_63_be(unsigned int n)
{
    uint64_t ret = 0;
    unsigned left = 0;
    if (!n)
        return 0;
    if (n > bits_valid) {
        left = bits_valid;
        n -= left;
        if (left)
            ret = bits_priv_val_get_be(left);
        if (bits_priv_refill_64_be() < 0)
            bits_valid = n;
    }
    ret = bits_priv_val_get_be(n) | ret << n;
    return ret;
}
/**

 * Return n bits from the buffer, n has to be in the 0-64 range.

 */
inline uint64_t bits_read_64_be(unsigned int n)
{
    if (n == 64) {
        uint64_t ret = bits_read_63_be(63);
        return (ret << 1) | (uint64_t)bits_read_bit_be();
    }
    return bits_read_63_be(n);
}
/**

 * Return n bits from the buffer as a signed integer, n has to be in the 1-32

 * range. May be faster than bits_read_signed() when n is not a compile-time

 * constant and is known to be non-zero;

 */
inline int32_t bits_read_signed_nz_be(unsigned int n)
{
    return sign_extend(bits_read_nz_be(n), n);
}
/**

 * Return n bits from the buffer as a signed integer.

 * n has to be in the 0-32 range.

 */
inline int32_t bits_read_signed_be(unsigned int n)
{
    if (!n)
        return 0;
    return bits_read_signed_nz_be(n);
}
/**

 * Return n bits from the buffer but do not change the buffer state.

 * n has to be in the 1-32 range. May

 */
inline uint32_t bits_peek_nz_be(unsigned int n)
{
    if (n > bits_valid)
        bits_priv_refill_32_be();
    return bits_priv_val_show_be(n);
}
/**

 * Return n bits from the buffer but do not change the buffer state.

 * n has to be in the 0-32 range.

 */
inline uint32_t bits_peek_be(unsigned int n)
{
    if (!n)
        return 0;
    return bits_peek_nz_be(n);
}
/**

 * Return n bits from the buffer as a signed integer, do not change the buffer

 * state. n has to be in the 1-32 range. May be faster than bits_peek_signed()

 * when n is not a compile-time constant and is known to be non-zero;

 */
inline int bits_peek_signed_nz_be(unsigned int n)
{
    return sign_extend(bits_peek_nz_be(n), n);
}
/**

 * Return n bits from the buffer as a signed integer,

 * do not change the buffer state.

 * n has to be in the 0-32 range.

 */
inline int bits_peek_signed_be(unsigned int n)
{
    if (!n)
        return 0;
    return bits_peek_signed_nz_be(n);
}
/**

 * Skip n bits in the buffer.

 */
inline void bits_skip_be(unsigned int n)
{
    if (n < bits_valid)
        bits_priv_skip_remaining_be(n);
    else {
        n -= bits_valid;
        bits = 0;
        bits_valid = 0;
        if (n >= 64) {
            unsigned int skip = n / 8;
            n -= skip * 8;
            ptr += skip;
        }
        bits_priv_refill_64_be();
        if (n)
            bits_priv_skip_remaining_be(n);
    }
}
/**

 * Seek to the given bit position.

 */
inline void bits_seek_be(unsigned pos)
{
    ptr = buffer;
    bits = 0;
    bits_valid = 0;
    bits_skip_be(pos);
}
/**

 * Skip bits to a byte boundary.

 */
inline const uint8_t *bits_align_be()
{
    unsigned int n = -bits_tell_be() & 7;
    if (n)
        bits_skip_be(n);
    return buffer + (bits_tell_be() >> 3);
}


};

// TODO: Benchmark and optionally enable on other 64-bit architectures.
using BitBuf = uint64_t ;
constexpr int BUF_BITS = 8 * sizeof(BitBuf);

struct PutBitContext {
    BitBuf bit_buf = 0;
    int bit_left = 0;
    uint8_t *buf = nullptr, *buf_ptr = nullptr, *buf_end = nullptr;
    std::vector<uint8_t> bytes;

    PutBitContext& operator=(const PutBitContext &other) {

        if (this == &other )
            return *this;
        bit_buf = other.bit_buf;
        bit_left = other.bit_left;
        buf = other.buf;
        buf_ptr = other.buf_ptr;
        buf_end = other.buf_end;
        bytes = other.bytes;
        if (buf)
            rebase_put_bits(bytes.data(), bytes.size());
        return *this;
    }

    PutBitContext(const PutBitContext &other) {
        operator=( other );
    }

/**
 * Initialize the PutBitContext s.
 *
 * @param buffer the buffer where to put bits
 * @param buffer_size the size in bytes of buffer
 */
PutBitContext()
{
    buf = nullptr;
    buf_end = nullptr;
    buf_ptr = nullptr;
    bit_left = BUF_BITS;
    bit_buf = 0;
}
/**
 * @return the total number of bits written to the bitstream.
 */
inline auto put_bits_count() const
{
    return (buf_ptr - buf) * 8 + BUF_BITS - bit_left;
}
/**
 * @return the number of bytes output so far; may only be called
 *         when the PutBitContext is freshly initialized or flushed.
 */
inline auto put_bytes_output() const
{
    return buf_ptr - buf;
}
/**
 * @param  round_up  When set, the number of bits written so far will be
 *                   rounded up to the next byte.
 * @return the number of bytes output so far.
 */
inline auto put_bytes_count(int round_up=true) const
{
    return buf_ptr - buf + ((BUF_BITS - bit_left + (round_up ? 7 : 0)) >> 3);
}
/**
 * Rebase the bit writer onto a reallocated buffer.
 *
 * @param buffer the buffer where to put bits
 * @param buffer_size the size in bytes of buffer,
 *                    must be large enough to hold everything written so far
 */
inline void rebase_put_bits(uint8_t *buffer,
                                   int buffer_size)
{
    if (!(8*buffer_size >= put_bits_count())) {
        throw std::logic_error("8*buffer_size >= put_bits_count(s)");
    }
    buf_end = buffer + buffer_size;
    buf_ptr = buffer + (buf_ptr - buf);
    buf = buffer;
}
/**
 * @return the number of bits available in the bitstream.
 */
inline auto put_bits_left()
{
    return (buf_end - buf_ptr) * 8 - BUF_BITS + bit_left;
}
/**
 * @param  round_up  When set, the number of bits written will be
 *                   rounded up to the next byte.
 * @return the number of bytes left.
 */
inline auto put_bytes_left(int round_up)
{
    return buf_end - buf_ptr - ((BUF_BITS - bit_left + (round_up ? 7 : 0)) >> 3);
}
/**
 * Pad the end of the output stream with zeros.
 */
inline void flush_put_bits()
{
    if (bit_left < BUF_BITS)
        bit_buf <<= bit_left;
    while (bit_left < BUF_BITS) {
        if (!(buf_ptr < buf_end)) {
           //throw std::length_error("s->buf_ptr < s->buf_end");
           bytes.resize(  std::max<size_t>(8, bytes.size() * 2 ) );
           rebase_put_bits(bytes.data(), bytes.size());
        }
        *buf_ptr++ = bit_buf >> (BUF_BITS - 8);
        bit_buf <<= 8;
        bit_left += 8;
    }
    bit_left = BUF_BITS;
    bit_buf = 0;
}
inline void flush_put_bits_le()
{
    while (bit_left < BUF_BITS) {
        if (!(buf_ptr < buf_end)) {
            //throw std::length_error("s->buf_ptr < s->buf_end");
           bytes.resize( bytes.size() + 4096 );
           rebase_put_bits(bytes.data(), bytes.size());
        }
        *buf_ptr++ = bit_buf;
        bit_buf >>= 8;
        bit_left += 8;
    }
    bit_left = BUF_BITS;
    bit_buf = 0;
}


/**
 * Write up to 32 bits into a bitstream.
 */

inline void put_bits(int n, BitBuf value)
{
    auto l_bit_buf = bit_buf;
    auto l_bit_left = bit_left;
    /* XXX: optimize */
    if (n < l_bit_left) {
        l_bit_buf = (l_bit_buf << n) | value;
        l_bit_left -= n;
    } else {
        l_bit_buf <<= l_bit_left;
        l_bit_buf |= value >> (n - l_bit_left);
        if (!(buf_end - buf_ptr >= sizeof(BitBuf))) {
           bytes.resize( bytes.size() + 4096 );
           rebase_put_bits(bytes.data(), bytes.size());
        }
        ((((union unaligned_64 *) (buf_ptr))->l) = (av_bswap64(l_bit_buf)));
        buf_ptr += sizeof(BitBuf);

        l_bit_left += BUF_BITS - n;
        l_bit_buf = value;
    }
    bit_buf = l_bit_buf;
    bit_left = l_bit_left;
}

inline void put_bits_le(PutBitContext *s, int n, BitBuf value)
{
    auto l_bit_buf = bit_buf;
    auto  l_bit_left = bit_left;
    l_bit_buf |= value << (BUF_BITS - l_bit_left);
    if (n >= l_bit_left) {
        if (!(buf_end - buf_ptr >= sizeof(BitBuf))) {
           bytes.resize( bytes.size() + 4096 );
           rebase_put_bits(bytes.data(), bytes.size());
        }
        ((((union unaligned_64 *) (buf_ptr))->l) = (l_bit_buf));
        buf_ptr += sizeof(BitBuf);

        l_bit_buf = value >> l_bit_left;
        l_bit_left += BUF_BITS;
    }
    l_bit_left -= n;
    bit_buf = l_bit_buf;
    bit_left = l_bit_left;
}
inline void put_sbits(int n, int32_t value)
{

    put_bits(n, av_mod_uintp2_c(value, n));
}

/**
 * Write up to 64 bits into a bitstream.
 */
inline void put_bits64(int n, uint64_t value)
{
    if (n <= 32)
        put_bits(n, value);
    else {
        uint32_t lo = value & 0xffffffff;
        uint32_t hi = value >> 32;
        put_bits(n - 32, hi);
        put_bits(32, lo);
    }
}
inline void put_sbits63(int n, int64_t value)
{
    put_bits64(n, (uint64_t)(value) & (~(UINT64_MAX << n)));
}
/**
 * Return the pointer to the byte where the bitstream writer will put
 * the next bit.
 */
inline uint8_t *put_bits_ptr(PutBitContext *s) const
{
    return buf_ptr;
}
/**
 * Skip the given number of bytes.
 * PutBitContext must be flushed & aligned to a byte boundary before calling this.
 */
inline void skip_put_bytes(int n)
{
    if (!(n <= buf_end - buf_ptr)) {
       //throw std::logic_error("n <= s->buf_end - s->buf_ptr");
           bytes.resize( ((bytes.size() + n + 4096) >> 12) << 12);
           rebase_put_bits(bytes.data(), bytes.size());

    }
    buf_ptr += n;
}
/**
 * Skip the given number of bits.
 * Must only be used if the actual values in the bitstream do not matter.
 * If n is < 0 the behavior is undefined.
 */
inline void skip_put_bits(int n)
{
    unsigned bits = BUF_BITS - bit_left + n;
    buf_ptr += sizeof(BitBuf) * (bits / BUF_BITS);
    bit_left = BUF_BITS - (bits & (BUF_BITS - 1));
}
/**
 * Change the end of the buffer.
 *
 * @param size the new size in bytes of the buffer where to put bits
 */
inline void set_put_bits_buffer_size(int size)
{
    if (!(size <= INT_MAX/8 - BUF_BITS)) {
        throw std::logic_error("size <= INT_MAX/8 - BUF_BITS");
    }
    buf_end = buf + size;
}
/**
 * Pad the bitstream with zeros up to the next byte boundary.
 */
inline void align_put_bits()
{
    put_bits( bit_left & 7, 0);
}

};
}
