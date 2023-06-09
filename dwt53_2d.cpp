//https://godbolt.org/z/K8dE115Mr
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string.h>
#include <vector>
#include <assert.h>
#include "dwt53_1d.h"
using namespace std;


static void dwt53_2d(const int *xy, int width, int height, int *LL_LH_HL_HH) {
        
    // Выполнение DWT по строкам

    for (int y=0; y<width*height; y+=width)        
        dwt53(xy+y, width, LL_LH_HL_HH+y, LL_LH_HL_HH + y + sizeof_L(width));

    vector<int> y_vec(height); 
    vector<int> y_lh(height); 
    // Выполнение DWT по столбцам
    for (int x = 0; x < width; x++) {
        //Делаем из столбца вектор
        for (int y=x, y_i=0; y<width*height; y_vec[y_i++] = LL_LH_HL_HH[ y ], y+=width );
        dwt53(y_vec.data(), height, y_lh.data(), y_lh.data() + sizeof_L(height) );
        //Возвращаем вектор в столбец
        for (int y=x, y_i=0; y<width*height; LL_LH_HL_HH[ y ] = y_lh[y_i++], y+=width );
    }

}

static void idwt53_2d(int *xy, int width, int height, const int *LL_LH_HL_HH) {
        

    vector<int> y_vec(height); 
    vector<int> y_lh(height); 
    // Выполнение IDWT по столбцам
    for (int x = 0; x < width; x++) {
        //Делаем из столбца вектор
        for (int y=x, y_i=0; y<width*height; y_lh[y_i++] = LL_LH_HL_HH[ y ], y+=width );
        idwt53(y_vec.data(), height, y_lh.data(), y_lh.data() + sizeof_L(height) );
        //Возвращаем вектор в столбец
        for (int y=x, y_i=0; y<width*height; xy[ y ] = y_vec[y_i++], y+=width );
    }
    y_vec.clear();
    y_lh.clear();

    vector<int> x_lh(width);

    // Выполнение IDWT по строкам
    for (int y=0; y<width*height; y+=width) {
        copy(xy+y, xy+y+width, x_lh.begin()); //TODO: inplace idwt transform
        idwt53(xy+y, width, x_lh.data(), x_lh.data() + sizeof_L(width));
    }

}


void print(int * data, int n=64, int m=8) {

    while (n--) {
        cout << setw(4) <<  *data++;
        if (n % m == 0) {
            cout << endl;
        }
        else
          cout << " ";
    }
    cout << endl;

}

int main() {
    int data[8*8];
    for (int i=0; i<64; i++)
    {
        double x = i % 8 * 2.3 - 8;
        double y = i / 8 * 2.3 - 8;
        double r = sqrt( x*x+y*y );
        //data[i] = r > 8.0 ? 0 : (int)r * 16 / 8;
        data[i] = (int)r*256/11;
        //data[i] = i % 8;
    }
    print(data);

    int transformed_data[8*8] = { 0 };

    fill(begin(transformed_data), end(transformed_data), 77);

    dwt53_2d(data, 8,8, transformed_data );

    print(transformed_data);

    idwt53_2d(data, 8,8, transformed_data );

    print(data);
    
}
