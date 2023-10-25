#pragma once

#include <iostream>
#include <fstream>
#include <cmath>
#include <utility>
#include <vector>
#include "utils.hpp"

    class Codebook {
        public:
        Codebook(int max_value, int step, int expa) {
            for (int x = 0, v = 0, progress = 0; x <= max_value && v <= max_value; x++) {
                codebook.push_back(v);
                //std::cout << v << " ";
                v += progress / expa + 1;
                progress = progress + step;
            }
            codebook.back() = max_value;
            create_lockup_table();
        }

        Codebook(std::initializer_list<int> il) : codebook(il) {
            create_lockup_table();
        }

        inline int lockup_index(int value) {
            int pos_value = std::min<int>( abs(value), lockup.size()-1 );
            return value < 0 ? -lockup[pos_value].first : lockup[pos_value].first;

        }
        inline int lockup_value(int value) {
            int pos_value = std::min<int>( abs(value), lockup.size()-1 );
            return value < 0 ? -lockup[pos_value].second : lockup[pos_value].second;
        }

        inline std::pair<int,int> lockup_pair(int value) {
            int pos_value = std::min<int>( abs(value), lockup.size()-1 );
            auto pair = lockup[pos_value];
            if (value < 0) {
                pair.first = -pair.first;
                pair.second = -pair.second;
            }
            return pair;
        }


        inline int get_codebook_value_at(int index) {
            int pos_index = std::min<int>( abs(index), codebook.size()-1 );
            return index < 0 ? -codebook[pos_index] :  codebook[pos_index];
        }
        inline int size() {
            return codebook.size();
        }

        private:
        void create_lockup_table() {
            const int max_value = codebook.back();
            for (int i=0; i<=max_value; ++i) {
                auto v = find_nerest(codebook.begin(), codebook.end(), i);
                lockup.emplace_back(std::distance(codebook.begin(), v), *v);
            }
        }

        std::vector<int> codebook;
        std::vector<std::pair<int,int>> lockup;

    };
