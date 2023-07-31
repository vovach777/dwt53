//this is snapshot of project here: https://godbolt.org/z/xdKn3WGq7
#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <iomanip>
#include <algorithm>

#define THE_MATRIX_DEFINED
using the_matrix = std::vector<std::vector<int>>;

#include "dwt53.hpp"


int calculatePSNR(const the_matrix & originalImage, const the_matrix& compressedImage)
{
    // Расчет суммы квадратов разностей пикселей
    auto mse = 0ULL;
    auto count = 0ULL;
    int maxPixelValue = 0;
    for (int y=0,max_y=std::min(originalImage.size(), compressedImage.size()); y<max_y; y++)
    for (int x=0,max_x=std::min(originalImage.size(), compressedImage.size()); x<max_x; x++)
    {
        auto diff = originalImage[y][x] - compressedImage[y][x];
        mse += diff * diff;
        count++;
        maxPixelValue = std::max(maxPixelValue, abs(originalImage[y][x]) );
        maxPixelValue = std::max(maxPixelValue, abs(compressedImage[y][x]) );
    }
    if (count == 0)
        return 0;
    // Расчет среднеквадратичной ошибки (MSE)
  

    if (mse == 0)
        return 100;
    
    // Расчет PSNR
    int psnr =  round ( 10.0 * log10((double)(maxPixelValue * maxPixelValue)*count / mse) );
    
    return psnr;
}



std::ostream& operator << (std::ostream& o,  std::vector<std::vector<int>>const & a)
{
    o << std::endl;
    for (auto & row : a)
    {
        for (auto v : row)
            o << std::setw(3) << v << " ";
        o << std::endl;
    }
    o << std::endl;
    return o;
}

template <typename T>
T square(const T v) {
   return v*v;
}

the_matrix make_sample(int size) {
   the_matrix data(size,std::vector<int>(size));
  for (int y=0; y<size; y++)
  for (int x=0; x<size; x++)
    {
        double xx =  x * 2.1 - size;
        double yy =  y * 2.1 - size;
        double r = sqrt( xx*xx+yy*yy );
        //data[i] = r > 8.0 ? 0 : (int)r * 16 / 8;
        data[y][x] = (int)r*256/size/2.1;
        //data[i] = i % 8;
    }
    return std::move(data);
}


the_matrix make_random(int size) {
    the_matrix data(size,std::vector<int>(size));
    for (int y=0; y<size; y++)
    for (int x=0; x<size; x++)
    {
     
        data[y][x] = rand() % 256;
        //data[i] = i % 8;
    }
    return std::move(data);
}

the_matrix make_monotonic(int size, int value)
{
    return std::move(the_matrix(size, std::vector<int>(size,value)));
}

the_matrix make_gradient(int size) {
   the_matrix data(size,std::vector<int>(size));
  for (int y=0; y<size; y++)
  for (int x=0; x<size; x++)
    {
     
        
        data[y][x] = 100 + (x+y)*100/(size*2);
        //data[i] = i % 8;
    }
    return std::move(data);
}



template <typename T>
static T median(T a, T b, T c) {
   if (a<b) {
      if (b<c)
         return b;
      return  (a < c) ? c : a;
   } else {
      if (b>=c)
         return b;
      return a < c ? a : c;
   }
}

unsigned matrix_power(const the_matrix& m)
{
    unsigned result = 0;
    for (const auto& row : m)
    for (const auto& v  : row)
    {
        result += abs(v);
    }
    return result;
}


static int r_shift( int num, int shift) {
    
    return (num < 0 ? -(-num >> shift) : num >> shift);
    
}

static double interpolate(
    const double bottomLeft, 
    const double topLeft, 
    const double bottomRight, 
    const double topRight,
    const double dx, 
    const double dy
) {
    return
        bottomLeft * (1 - dx) * (1 - dy) +
        topLeft    * (1 - dx) * dy +
        bottomRight * dx * (1 - dy) +
        topRight   * dx * dy;
}

the_matrix make_gradient(int width, int height, int topLeft, int topRight, int bottomLeft, int bottomRight) {
    the_matrix a( height, std::vector<int>(width,0));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double dx = static_cast<double>(x) / (width - 1);
            double dy = static_cast<double>(y) / (height - 1);
            a[y][x] = interpolate(topLeft, bottomLeft, topRight,bottomRight, dx, dy);
        }
    }
    return a;
}

int main() {

    int details = 2;
    int max_levels = 5;
    int Q = 1;
 
    auto data = make_gradient(27,17,0,44,55,88);
    auto original = data;
    std::cout << "original:" << original;
    compress_data(data, max_levels, Q, details);
    std::cout << "compressed:" << data;
    decompress_data(data, max_levels, Q, details);
    std::cout << "decompressed:" << data;
    std::cout << "psnr=" << calculatePSNR(original,data) << std::endl;

   

   
}
