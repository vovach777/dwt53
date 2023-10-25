//https://raw.githubusercontent.com/vovach777/huffman_advanced/main/huffman.hpp
#pragma once
#include <algorithm>
#include <bitset>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <ostream>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include "bitstream.hpp"
#include "utils.hpp"
#include "bitstream_helper.hpp"

namespace pack {
    namespace huffman_impl {
    using namespace std;
    // A Huffman
    template <typename T = char>
    class Huffman {
        // A Huffman node
        struct Node {
            T ch;
            int freq;
            Node *left, *right;
            // Function to allocate a new tree node
            Node(T ch, int freq, Node* left, Node* right)
                : ch(ch), freq(freq), left(left), right(right) {}
            bool operator<(const Node& other) const {
                return freq < other.freq;
            }
        };

        // Comparison object to be used to order the heap

        struct Node_comp {
            bool operator()(Node* l, Node* r) {
                // highest priority item has lowest frequency
                return l->freq > r->freq;
            }
        };

        Node* root;
        vector<Node> pool;
        using BitCountType = uint8_t;
        using HuffmanCodeType = uint32_t;
        unordered_map<T, pair<BitCountType, HuffmanCodeType>> huffmanCode;
        vector<vector<T>> DHT;

        // traverse the Huffman Huffman and store Huffman Codes
        // in a map.
        void fillDHT(Node* node, int len) {
            if (node == nullptr) return;

            // found a leaf node
            if (!node->left && !node->right) {
                //assert(len <= 16 && len > 0);
                if (DHT.size() < len) DHT.resize(len);
                DHT[len - 1].push_back(node->ch);
            }

            fillDHT(node->left, len + 1);
            fillDHT(node->right, len + 1);
        }

       public:
        std::string exportDHT()
        {
            ostringstream out;
            out << "vector<vector<T>>{"  << endl;
            for (auto & row : DHT)
            {
                out << "{";
                    for (auto item : row)
                    {
                        out << int64_t(item) << ",";
                    }
                out << "}," << endl;
            }
            out << "};" << endl;
            return out.str();

        }
        void setDHT( const vector<vector<T>> & dht )
        {
            DHT = dht;
            create_lockup_table();  // need for encoder
            buildHuffmanTree();     // need for decoder
        }

        void encodeHuffmanTree(BitWriter& dest) {
            if (DHT.size() == 0) throw std::domain_error("No DHT found!");
            //std::cout << std::endl;
            dest.set_ur_golomb(DHT.size(),5,10,16);
            for (int i = 0; i < DHT.size(); i++) {
                ///dest.writeBits(8, DHT[i].size());
                dest.set_ur_golomb(DHT[i].size(),3,10,16);
                if ( DHT[i].size() == 0)
                    continue;
                //cout << i << " (" << DHT[i].size() << ") :";
                for (auto symbol : DHT[i]) {
                    auto catindex = to_jpair(symbol);
                    dest.writeBits(4, catindex & 0xf);
                    if ( catindex > 0)
                        dest.writeBits(catindex & 0xf, catindex >> 4);
                    //cout << symbol << ",";
                }
                //std::cout << std::endl;
            }
        }

        void decodeHuffmanTree(BitReader& src) {
            // decode huffman table
            DHT.clear();
            //DHT.resize(src.readBits(8));
            DHT.resize(src.get_ur_golomb(5,10,16));
            for (int i = 0; i < DHT.size(); i++) {
                //DHT[i].resize(src.readBits(8));
                DHT[i].resize(src.get_ur_golomb(3,10,16));
                if (DHT[i].size() == 0) continue;
                //cout << i << " (" << DHT[i].size() << ") :";
                // cout << "DHT_" << i << ":" << DHT[i].size() << endl;
                for (auto& value : DHT[i]) {
                    auto cat = src.readBits(4);
                    auto index = cat == 0 ? 0 : src.readBits(cat);
                    value = from_jpair( make_jpair(cat, index) );
                    //cout << value << ",";
                }
                //cout << endl;
            }
            //cout << endl;
            create_lockup_table();  // need for encoder
            buildHuffmanTree();     // need for decoder
        }
        void create_lockup_table() {
            //std::cout << "generation huffman codes:" << endl;
            huffmanCode.clear();
            HuffmanCodeType code = 0;
            for (int bits = 0; bits < DHT.size(); ++bits) {
                for (int i = 0; i < DHT[bits].size(); i++) {
                    // HuffmanCodeType rev_code = 0;
                    // for (int n = bits-1; n >= 0; n--){
                    //     rev_code |= (code >> n) & 1;
                    //     rev_code <<= 1;
                    // }

                    huffmanCode[DHT[bits][i]] = make_pair(bits + 1, code);
                    code++;
                }
                code <<= 1;
            }
            //cout << "Huffman Codes are :\n" << '\n';
            // for (const auto& pair : huffmanCode) {
            //     std::bitset<std::numeric_limits<HuffmanCodeType>::digits> bs(
            //         pair.second.second);
            //     auto s = bs.to_string();
            //     cout << setw(4) << pair.first << " "
            //          << s.substr(s.size() - pair.second.first,
            //                      pair.second.first)
            //          << " (" << (int)pair.second.first << ","
            //          << pair.second.second << ")" << endl;
            // }
        }
        // Builds Huffman Huffman
        template <typename Iterator>
        auto buildHuffmanTree(Iterator begin, Iterator end)
            -> decltype(std::enable_if_t<
                            std::is_same_v<typename std::iterator_traits<
                                               Iterator>::value_type,
                                           T>,
                            Iterator>(),
                        void()) {
            // count frequency of appearance of each character
            // and store it in a map
            unordered_map<T, size_t> freq;
            for (auto it = begin; it != end; ++it) {
                freq[*it]++;
            }
            buildHuffmanTree(freq);

        }

        void buildHuffmanTree(const unordered_map<T, size_t> & freq)
        {

            pool.clear();
            pool.reserve(freq.size() * 2);

            // Create a priority queue to store live nodes of
            // Huffman tree;

            priority_queue<Node*, vector<Node*>, Node_comp> pq;

            // Create a leaf node for each character and add it
            // to the priority queue.
            for (auto pair : freq) {
                pq.push(&pool.emplace_back(pair.first, pair.second, nullptr,
                                           nullptr));
            }

            // do till there is more than one node in the queue
            while (pq.size() != 1) {
                // Remove the two nodes of highest priority
                // (lowest frequency) from the queue
                Node* left = pq.top();
                pq.pop();
                Node* right = pq.top();
                pq.pop();

                // Create a new internal node with these two nodes
                // as children and with frequency equal to the sum
                // of the two nodes' frequencies. Add the new node
                // to the priority queue.
                int sum = left->freq + right->freq;
                pq.push(&pool.emplace_back('\0', sum, left, right));
            }

            // root stores pointer to root of Huffman Huffman
            root = pq.top();

            // traverse the Huffman Huffman and store (not Huffman Codes) bitlength
            // of each symbol in a map. Also prints them
            DHT.clear();
            DHT.reserve(16);
            fillDHT(root, 0);

            // huffman table not needed any more - lockup table to be
            // regenerated from DHT
            root = nullptr;
            pool.clear();
            create_lockup_table();
            buildHuffmanTree();
        }
        // from lockuptable
        void buildHuffmanTree() {
            if (huffmanCode.size() == 0) {
                root = nullptr;
                return;
            }
            pool.clear();
            pool.reserve(huffmanCode.size() * 2);
            root = &pool.emplace_back('\0', 1, nullptr, nullptr);

            // for (const auto & symbols : DHT)
            // for (const auto symbol : symbols) {
            //     const auto& code = huffmanCode[symbol];
            //     cout << symbol << " " << code<< '\n';
            for (const auto& pair : huffmanCode) {
                auto symbol = pair.first;
                // cerr << endl << "(" << symbol << ") : ";
                auto& code = pair.second;
                auto n = root;
                for (int i = code.first - 1; i >= 0; --i) {
                    auto v = (code.second >> i) & 1;
                    // cerr << v;
                    if (v == 0) {
                        if (!n->left) {
                            n->left =
                                &pool.emplace_back('\0', 1, nullptr, nullptr);
                        }
                        assert(n->freq == 1);
                        n = n->left;

                    } else {
                        if (!n->right) {
                            n->right =
                                &pool.emplace_back('\0', 1, nullptr, nullptr);
                        }
                        assert(n->freq == 1);
                        n = n->right;
                    }
                }
                assert(n->left == nullptr && n->right == nullptr &&
                       n->freq == 1);
                n->ch = symbol;
                n->freq = 0;
            }
            // cerr << endl;
        }

        template <typename Iterator>
        void encode(BitWriter& dest, Iterator begin, Iterator end) {
            for (auto it = begin; it != end; ++it) {
                encode(dest, *it);
            }
            dest.flush();
            //std::cerr << "huffman_codes: " << dest << std::endl;
        }
        inline void encode(BitWriter& dest, T value) {
            auto [bit_count, code] = huffmanCode.at(value);
            // while (bit_count--) {
            //     dest.writeBit(code & 1);
            //     code >>= 1;
            // }
            //std::cout << "encoding " << (int)value << " => (" << (int)bit_count << "," << (int)code << ")" << std::endl;
            dest.writeBits(bit_count, code);
        }

        // traverse the Huffman Huffman and decode the encoded string
        T decode(BitReader& src) {
            if (root == nullptr) {
                throw std::domain_error("No the huffman-tree found!");
            }

            auto n = root;

            for (;;) {
                // if (src.bit_left() == 0)
                //     return root->ch;

                if (src.readBit() == false) {
                    n = n->left;
                } else {
                    n = n->right;
                }
                // found a leaf node
                if (n->freq == 0) {
                    // cerr << "(" << n->ch << ")";
                    return n->ch;
                }
            }
        }
        template <typename Iterator>
        void decode(BitReader& src, Iterator begin, Iterator end) {
            for (auto it = begin; it < end; it++) {
                *it = decode(src);
            }
        }
    };
    }
    //export section
    using huffman_impl::Huffman;
}
