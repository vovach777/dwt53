#include "huffman.h"
#include "huffmanpack.hpp"

namespace huffman {

std::vector<uint8_t> compress(const the_matrix& matrix) {
    std::vector<int> histogram;
    process_matrix(matrix, [&histogram](int v) {
        auto value = (v < 0) ? -v : v;
        if (value >= histogram.size()) {
            histogram.resize(value + 1, 0);
        }
        histogram[value]++;
    });
    // std::cout << "histogram:" << histogram;
    auto henc = new_Hufftree(histogram.data(), histogram.size());
    PutBitContext pb = {0};
    put_bits32(&pb, matrix[0].size());
    put_bits32(&pb, matrix.size());
    auto tree_size = (encodetree_Hufmantree(henc, &pb) + 7) / 8;
    //std::cerr << "huffman tree size = " << tree_size << std::endl;
    // huffman encoding...
    process_matrix(matrix, [pb = &pb, henc](int v) {
        auto value = (v < 0) ? -v : v;
        encode_Hufftree(henc, value, pb);
        put_bits(pb, 1, !!(v < 0));
    });

    auto packed_size = put_bytes_count(&pb);
    //std::cerr << "packed size = " << packed_size << std::endl;
    delete_Hufftree(henc);
    std::vector<uint8_t> result(pb.buf, pb.buf + packed_size);
    free(pb.buf);
    return result;
}

the_matrix decompress(const std::vector<uint8_t>& compressed) {
    /* decompression here */
    GetBitContext gb = {0};
    init_get_bits(&gb, compressed.data(), compressed.size());

    int width = get_bits_long(&gb, 32);
    int height = get_bits_long(&gb, 32);
    Hufftree* decoder = new_Hufftree2(&gb);

    the_matrix result = make_matrix(width, height, [gb = &gb,decoder](int& v) {
        const int unsigned_value = decode_Hufftree(decoder, gb);
        v = get_bits1(gb) ? -unsigned_value : unsigned_value;
    });

    delete_Hufftree(decoder);

    return result;
}

};  // namespace huffman
