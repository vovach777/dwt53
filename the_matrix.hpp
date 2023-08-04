#ifndef THE_MATRIX_DEFINED
   #define THE_MATRIX_DEFINED

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <algorithm>
#include <ostream>
#include <iomanip>
#include <cassert>


struct Index {
    int offs;
    int size;
};

struct Block {
    Index x;
    Index y;
};


using the_matrix = std::vector<std::vector<int>>;
static bool raster_flag = false;

static int make_positive(the_matrix & a);
static inline int getValue(const the_matrix& matrix, int row, int col);


static the_matrix const & raster(the_matrix const& img) {
    raster_flag = true;
    return img;
}

struct Range {
    int min;
    int max;
};

Range get_range(the_matrix const &matrix)
{
    int minVal = std::numeric_limits<int>::max();
    int maxVal = std::numeric_limits<int>::min();

    for (const auto& row : matrix) {
        for (const auto val : row) {
            minVal = std::min(minVal, val);
            maxVal = std::max(maxVal, val);
        }
    }
    return Range{minVal,maxVal};
}


Range get_range(the_matrix const &matrix, Block const& block)
{
    int minVal = std::numeric_limits<int>::max();
    int maxVal = std::numeric_limits<int>::min();
    
    for (int j=0; j < block.y.size; j++)
    for (int i=0; i < block.x.size; i++)
    {
        auto val = getValue(matrix, block.x.offs + i, block.y.offs + j);
        minVal = std::min(minVal, val);
        maxVal = std::max(maxVal, val);        
    }
    return Range{minVal,maxVal};
}


void raster(std::ostream& o,the_matrix const& img)
{
    static char pixels[] = {' ', '-', '+' ,'=','#','@'};
    auto range = get_range(img);
    

      for (const auto & row : img) {
      for (const auto v : row)
      {
         auto pixel = pixels[ std::clamp<int>(  (v - range.min) * sizeof(pixels) / (range.max - range.min),0,sizeof(pixels)-1) ];
         o <<  pixel << pixel << pixel ;
      }
        o <<std::endl;
      }
}


static std::ostream& operator << (std::ostream& o,  the_matrix const & a)
{
     o << std::endl;
    if (raster_flag) {
        raster_flag = false;
        raster(o, a);
    }
    else {
        for (auto & row : a)
        {
            for (auto v : row)
                o << std::setw(3) << v << " ";
            o << std::endl;
        }
    }
    o << std::endl;
    return o;


}


static void transpose(the_matrix& matrix) {
    const auto rows = matrix.size();
    const auto cols = matrix[0].size();

    if (rows != cols) {
        // Матрица не квадратная, выполняем перемещение
        the_matrix transposed(cols, std::vector<int>(rows));

        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                transposed[j][i] = matrix[i][j];
            }
        }

        std::swap(matrix,transposed);
    } else {
        // Матрица квадратная, выполняем обмен элементами
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < i; ++j) {
                std::swap(matrix[i][j], matrix[j][i]);
            }
        }
    }
}

std::vector<int> get_col(the_matrix const &matrix, int x) {
    std::vector<int> col(matrix.size());
    for (int i=0; i< col.size(); i++ )
        col[i] = matrix[i][x];
    return col;
}

void set_col(the_matrix  &matrix, int x,  std::vector<int> const & col) {
    assert( col.size() == matrix.size() );
    
    for (int i=0; i< col.size(); i++ )
        matrix[i][x] = col[i];
    
}


static inline void setValue( the_matrix& matrix, int row, int col, int value) {
    int height = matrix.size();
    int width = matrix[0].size();


    if (row >= 0 && row < height && col >= 0 && col < width)
        matrix[row][col] = value;
}


static void drawLine(the_matrix& matrix, int x1, int y1, int x2, int y2, int value = 1) {
    int width = matrix[0].size();
    int height = matrix.size();

    int dx = x2-x1;
    int dy = y2-y1;
    double m = double(dy)/dx;
    double xin,yin;
    int tot;
    if(dx==0)
    {
        if(y1>y2) std::swap(y1,y2);
        for(int i=y1;i<=y2;i++) setValue(matrix,i,x1,value);
        return ;
    }
    else if(dy==0)
    {
        if(x1>x2) std::swap(x1,x2);
        for(int i=x1;i<=x2;i++) setValue(matrix,y1,i,value);
        return ;
    }

    if(abs(dx)>=abs(dy))
    {
        if(x1>x2)
        {
            std::swap(x1,x2);
            std::swap(y1,y2);
        }
        xin = 1;
        yin = m;
        tot = x2-x1+1;
    }
    else
    {
        if(y1>y2)
        {
            std::swap(x1,x2);
            std::swap(y1,y2);
        }
        yin = 1;
        xin = 1.0/m;
        tot = y2-y1 + 1;

    }
    double x = x1;
    double y = y1;
    for(int i=1;i<=tot;i++)
    {
        setValue(matrix,round(y),round(x),value);
        x += xin;
        y += yin;
    }

}


static inline int getValue(const the_matrix& matrix, int row, int col) {
    int height = matrix.size();
    int width = matrix[0].size();

    // Удерживаем координаты в границах матрицы
    row = std::max(0, std::min(height - 1, row));
    col = std::max(0, std::min(width - 1, col));

    return matrix[row][col];
}

static void cubicBlur3x3(the_matrix& matrix) {
    int height = matrix.size();
    int width = matrix[0].size();


    // Копирование исходной матрицы для сохранения оригинальных значений
    the_matrix originalMatrix = matrix;

    // Применение кубического размытия
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int sum = 0;
            for (int yy = y-1; yy <= y+1; yy++) {
                for (int xx = x-1; xx <= x+1; xx++) {
                    sum += getValue(originalMatrix, yy, xx);
                }
            }
            matrix[y][x] = (sum+5) / 9;
        }
    }
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


static the_matrix make_envelope(int width, int height, int value)
{
    the_matrix result(height, std::vector<int>(width, 0));
    drawLine(result, 0, 0, width-1, 0, value);
    drawLine(result, width-1, 0, width-1, height-1, value);
    drawLine(result, width-1, height-1, 0, height-1, value);
    drawLine(result, 0, height-1, 0, 0, value);
    drawLine(result, 0, 0, width-1, height-1, value);
    drawLine(result, 0, height-1, width-1, 0, value);
    return result;
}


static int psnr(const the_matrix & originalImage, const the_matrix& compressedImage)
{
    // Расчет суммы квадратов разностей пикселей
    auto mse = 0ULL;
    auto count = 0ULL;
    int maxPixelValue = 0;
    for (int y=0,max_y=std::min(originalImage.size(), compressedImage.size()); y<max_y; y++)
    for (int x=0,max_x=std::min(originalImage[y].size(), compressedImage[y].size()); x<max_x; x++)
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

template<typename F>
inline auto process_point(int x, int y, int &value, F f) -> decltype(f(value),void())
{
   f(value);
}

template<typename F>
inline auto process_point(int x, int y, int &value, F f) -> decltype(f(x,y,value), void())
{
   f(x,y,value);
}

template <typename F>
void process_matrix(the_matrix & matrix, F f)
{    
    for (int y=0; y<matrix.size();y++)
    for (int x=0; x<matrix[y].size();x++)
        process_point(x,y,matrix[y][x],f);    
}


template <typename F>
void process_matrix(the_matrix &matrix, Block const& block, F f)
{
    for (int j=0; j < block.y.size; j++)
    for (int i=0; i < block.x.size; i++)
    {
        int x = block.x.offs+i;
        int y = block.y.offs+j;
        process_point(x,y,matrix.at(y).at(x),f);    
    }
}


#endif