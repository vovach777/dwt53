//https://godbolt.org/z/Yva1x9n5h
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string.h>
#include <vector>
#include "dwt53_1d.h"
using namespace std;


int main() {


    for (int iter=0; iter<10000; ++iter)
    {
        int size = max( rand() % 64, 5);
        vector<int> data(size);
        for (auto& v : data) {
            v = rand() % 256;
        }
        vector<int> original = data;

        vector<int> L( sizeof_L(data.size()));
        vector<int> H( sizeof_H(data.size()));
        dwt53(data.data(), data.size(),  L.data(), H.data());
        idwt53(data.data(), data.size(),  L.data(), H.data());
        for (int i=0; i<size; ++i) {
            if ( data[i] != original[i] ) {
                cerr << "test failed!" << endl;
                return 1;
            }
        }
    }

    cout << "test passed!" << endl;
    
}
