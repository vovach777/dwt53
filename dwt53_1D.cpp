//https://godbolt.org/z/hcv9ef9Gd
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
    if (size & 1) {
        //C-variant
        int*H_p=H;
        const int*x_p=x+1;
        int s=size/2;        
        while (s--) {
            *H_p++ = x_p[0] - (x_p[-1] + x_p[1])/2;
            x_p += 2;
        }        
        int *L_p = L;
        //A-variant
        *L_p++ = x[0]+(H[0] + 1)/2;
        //D-variant
        s = (size+1)/2-2;
        x_p=x+2;
        H_p = H;
        while (s--) {
            *L_p++ = x_p[0] + (H_p[0] + H_p[1] + 2) / 4;
            H_p+=1;
            x_p+=2;
        }
        //B-variant
        *L_p++ = x_p[0]+(H_p[-1] + 1) /2; 

    } else {
        //C-variant
        int s=size/2-1;
        int *H_p=H;
        const int *x_p=x+1;
        while (s--) {
            *H_p++ = x_p[0] - (x_p[-1] + x_p[1])/2;
            x_p += 2;
        }
        //E-variant
        *H_p=x_p[0]-x_p[-1];
        s=size/2-1;
        int *L_p=L;
        H_p=H;
        //A-variant
        *L_p++ = x[0] + (H_p[0]+1)/2;
        //D-variant
        x_p = x + 2;
        while (s--) {
            *L_p++ = x_p[0] + (H_p[0] + H_p[1] + 2) / 4;
            H_p+=1;
            x_p+=2;
        }
    }
}


static void idwt53(int *x, int size, const int *L, const int *H) 
{    
    if (size & 1) {
        const int *L_p = L;
        //A-variant
        //*L_p++ = x[0]+(H[0] + 1)/2;
        x[0] = *L_p++ -(H[0] + 1)/2;
        //D-variant
        int s = (size+1)/2-2;
        int * x_p=x+2;
        const int* H_p = H;
        while (s--) {
            //*L_p++ = x_p[0] + (H_p[0] + H_p[1] + 2) / 4;
            x_p[0] = *L_p++ - (H_p[0] + H_p[1] + 2) / 4;
            H_p+=1;
            x_p+=2;
        }
        //B-variant
        //*L_p++ = x_p[0]+(H_p[-1] + 1) /2; 
        x_p[0] = *L_p++ - (H_p[-1] + 1) /2; 

        //C-variant
        H_p=H;
        x_p=x+1;
        s=size/2;        
        while (s--) {
            //*H_p++ = x_p[0] - (x_p[-1] + x_p[1])/2;
            x_p[0] = *H_p++ + (x_p[-1] + x_p[1])/2;
            x_p += 2;
        }        
    } else {
        //LLLLLL
        //////
        int s=size/2-1;
        const int *L_p=L;
        const int *H_p=H;
        //A-variant
        //*L_p++ = x_p[0] + (H_p[0]+1)/2;
        x[0] = *L_p++ - (H_p[0]+1)/2;
        //D-variant
        int *x_p = x + 2;
        while (s--) {
            //*L_p++ = x_p[0] + (H_p[0] + H_p[1] + 2) / 4;
            x_p[0] =  *L_p++ - (H_p[0] + H_p[1] + 2) / 4;
            H_p+=1;
            x_p+=2;
        }
        //HHHHHHH
        //C-variant
        s=size/2-1;
        H_p=H;
        x_p=x+1;
        while (s--) {
            //*H_p++ = x_p[0] - (x_p[-1] + x_p[1])/2;
            x_p[0] = *H_p++ + (x_p[-1] + x_p[1])/2;
            x_p += 2;
        }
        //E-variant
        //*H_p=x_p[0]-x_p[-1];
        x_p[0] = *H_p+x_p[-1];

    }
}


int main() {


    for (int iter=0; iter<10000; ++iter)
    {
        int size = max( rand() % 1920, 8);
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
