//https://godbolt.org/z/KPKTqz61M
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string.h>
#include <vector>
using namespace std;

int sizeof_L(int x)
{
    return (x+1) >> 1;
}

int sizeof_H(int x)
{
    return x >> 1;
}


static void dwt53(const int *x, int size, int *L, int *H) 
{
    // predict 1
    
    for (int i=1,j=0; i < size; i+=2, ++j ) {
        const int _b = x[i-1];
        const int b_ = i+1 < size ? x[i+1] : _b;
        H[j] = x[i] - (_b + b_)/2;        
    }
    // update 1    
    for (int i=0,j=0,j_size=sizeof_H(size); i < size; i+=2, ++j) {
        const int _a = H[j];
        const int a_ = H[min(j+1,j_size) ];
        L[j] = x[i] + (_a + a_ + 2)/4;
    }
}


static void idwt53(int *x, int size, const int *L, const int *H) 
{

    // update 1    
    for (int i=0,j=0,j_size=sizeof_H(size); i < size; i+=2, ++j) {
        const int _a = H[j];
        const int a_ = H[min(j+1,j_size) ];
        //L[j] = x[i] + (_a + a_ + 2)/4;
        x[i] =  L[j] - (_a + a_ + 2)/4;
    }

    // predict 1
    for (int i=1,j=0; i < size; i+=2, ++j ) {
        const int _b = x[i-1];
        const int b_ = i+1 < size ? x[i+1] : _b;
        //H[j] = x[i] - (_b + b_)/2; 
        x[i] = H[j] + (_b + b_)/2;
    }
}


int main() {


    for (int iter=0; iter<10000; ++iter)
    {
        int size = max( rand() % 64, 8);
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
