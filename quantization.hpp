#pragma once
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>
#include "utils.hpp"

class Codebook {
    std::vector<int> codebook;
    std::vector<int> value_to_cw;

    template <typename Iterator>
    void process_range(Iterator begin, Iterator end, Iterator zero_coefficient) {
        if (begin == end) return;
        auto bin_length = std::distance(begin, end);  // length of bin
        if (bin_length == 1) {
            if (*begin > 0) *begin = -*begin;
            return;
        }

        auto maxIt = std::max_element(begin, end);
        *maxIt = -*maxIt;
        auto clear_left = maxIt == begin ? end : maxIt - 1;
        auto clear_right = maxIt + 1;

        auto dynamic_bin = std::max<int>(1, std::distance(zero_coefficient, maxIt) / 4);

        for (int i = 1; i < dynamic_bin; i++) {
            if (clear_left != end) {
                *clear_left = 0;
                if (clear_left == begin)
                    clear_left = end;
                else
                    --clear_left;
            }
            if (clear_right != end) {
                *clear_right = 0;
                ++clear_right;
            }
        }

        if (clear_left != end) process_range(begin, clear_left, zero_coefficient);
        process_range(clear_right, end, zero_coefficient);
    }

    void create_codeword_index() {
        value_to_cw.clear();
        for (int i = 0; i < codebook.back(); i++) {
            value_to_cw.push_back(std::distance(
                codebook.begin(),

                find_nerest(codebook.begin(), codebook.end(), i)));
        }
        //std::cerr << "create_codeword_index:" << value_to_cw;
    }

   public:
    Codebook(){};
    Codebook(Codebook&& other) noexcept
        : codebook(std::move(other.codebook)),
          value_to_cw(std::move(other.value_to_cw)) {}

    Codebook(std::vector<int> hist) {
        //std::cerr << "histogram=" << hist;

        process_range(hist.begin() + 4, hist.end(), hist.begin());

        for (int i = 0; i < hist.size(); ++i) {
            if (hist[i]) {
                codebook.push_back(i);
            }
        }
        create_codeword_index();
    }
    void set_external_codebook(std::vector<int> external) {
        codebook = std::move(external);
        create_codeword_index();
    }

    Codebook(int max_value, int step = 5, int expa = 16) {
        for (auto x = 0, v = 0, progress = 0; x <= max_value && v < max_value; x++) {
            codebook.push_back(v);
            v += progress / expa + 1;
            progress = progress + step;
        }
        create_codeword_index();
    }

    //Codebook() : Codebook(256, 5, 16) {}

    int get_codeword(int v) {
        if (is_empty())
            return v; //by-pass
        int max_value = static_cast<int>(value_to_cw.size() - 1);
        if (v <= -max_value) return -value_to_cw.back();
        if (v >= max_value) return value_to_cw.back();
        if (v < 0)
            return -value_to_cw[-v];
        else
            return value_to_cw[v];
    }

    int get_value(int cw) {
        if (is_empty())
            return cw; //by-pass
        int max_value = static_cast<int>(codebook.size() - 1);
        if (cw <= -max_value) return -codebook.back();
        if (cw >= max_value) return codebook.back();
        if (cw < 0)
            return -codebook[-cw];
        else
            return codebook[cw];
    }

    void clear() {
        codebook.clear();
        value_to_cw.clear();
    }
    bool is_empty() {
        return codebook.size() == 0;
    }
    int size() { return codebook.size(); }
    int at(int cw) { return codebook.at(cw); }
    const std::vector<int>& get_codebook() {
        return codebook;
    }

    Codebook& operator=(Codebook&& other) noexcept {
        if (this != &other) {
            codebook = std::move(other.codebook);
            value_to_cw = std::move(other.value_to_cw);
        }
        return *this;
    }
};
