#pragma once
#include <vector>
#include "the_matrix.hpp"
namespace huffman {
std::vector<uint8_t> compress(const the_matrix& matrix);
the_matrix decompress(const std::vector<uint8_t>& compressed);
};