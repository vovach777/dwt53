#ifndef THE_MATRIX_DEFINED
   #define THE_MATRIX_DEFINED

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <algorithm>
#include <ostream>
#include <iomanip>

using the_matrix = std::vector<std::vector<int>>;

static std::ostream& operator << (std::ostream& o,  the_matrix const & a)
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

static the_matrix make_radial(int size) {
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
    return data;
}


static the_matrix make_random(int size) {
    the_matrix data(size,std::vector<int>(size));
    for (int y=0; y<size; y++)
    for (int x=0; x<size; x++)
    {

        data[y][x] = rand() % 256;
        //data[i] = i % 8;
    }
    return data;
}

static the_matrix make_monotonic(int size, int value)
{
    return the_matrix(size, std::vector<int>(size,value));
}

static the_matrix make_gradient(int size) {
   the_matrix data(size,std::vector<int>(size));
  for (int y=0; y<size; y++)
  for (int x=0; x<size; x++)
    {
        data[y][x] = 100 + (x+y)*100/(size*2);
        //data[i] = i % 8;
    }
    return data;
}


static std::vector<double> make_sin(int N, int freq = 1, double phase_in_T=0) {

    std::vector<double> vec;

    int phase_in_N = (int)round( N * fmod( phase_in_T, 1) / freq) % N;
    for (int i=0; i<N; i++)
        vec.push_back( std::sin( 2 * M_PI * freq / N * ( (i+phase_in_N) % N)  ) );
    return vec;
}

static auto matrix_energy(const the_matrix& m)
{
   auto elements_num = m.size()*m[0].size();
   decltype(elements_num) result = 0;
    if ( elements_num > 0)
    {
      for (const auto& row : m)
      for (const auto& v  : row)
      {
         const auto energy = abs(v)*2;
         result += energy;
      }
      result = result * 1000 / elements_num;
    }
    return result;
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

static the_matrix make_gradient(int width, int height, int topLeft, int topRight, int bottomLeft, int bottomRight) {
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


static int make_positive(the_matrix & a)
{
    int min = 0;
    for (const auto & row: a)
    for (const auto v : row)
       min = std::min(v, min);
    if (min < 0) {
    for (auto & row: a)
    for (auto & v : row)
       v -= min;
    }
    return min;
}

static the_matrix make_sin2d(int width, int height, double freq, int time) {
    the_matrix gradient = the_matrix(height, std::vector<int>(width,0));
    double freq_x = freq + sin(time/16.0);
    double freq_y = freq_x / 2;
    double amp_y = 12;
    for (int y=0, time_y=time; y < height; ++y, ++time_y, amp_y *= 0.98 ) {
        double amp_x = 64;

        int y_ = static_cast<int>( std::sin( 2 * M_PI * freq_y / height * (time_y % height)) * amp_y );
        for (int t = time+width, x=0; x < width; --t,++x, amp_x *= 0.98  )
        {
            ////gradient[y][x]  = y_ - static_cast<int>( y * 0.2 );
            gradient[y][x]  = y_  +  static_cast<int>( std::sin( 2 * M_PI * freq_x / width * (t % width)) * amp_x ) + 128;
        }
    }
   // make_positive(gradient);
    return gradient;
}



static int psnr(const the_matrix & originalImage, const the_matrix& compressedImage)
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

#endif