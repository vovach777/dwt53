// original development is here: https://godbolt.org/z/hMhMYfbTY
#pragma once
/*
  * variants *
        ** L - variants **
        A: xLH
        B: HLx
        D: HLH
        
        ** H - variants **
        C: LHL
        E: LHx
        
        ** ODD N=7 **
        LHLHLHL => H-variants:  LHL (C);   L-variants:  xLH (A), HLH (D), HLx (B)
        
        ** EVEN N=8 **
        LHLHLHLH => H-variants: LHL (C), LHx (E); L-variants: xLH (A), HLH (D)
        
        ** Formulas (forward) **
        H, L
        C: H = H - (L0 + L1) / 2
        E: H = H - L0
        D: L = L + (H0 + H1 + 2) / 4
        B: L = L + (H0 + 1) / 2
        A: L = L + (H1 + 1) / 2
        ** Formulas (inverse) **
        L, H
        D: L = L - (H0 + H1 + 2) / 4
        B: L = L - (H0 + 1) / 2
        A: L = L - (H1 + 1) / 2
        C: H = H + (L0 + L1) / 2
        E: H = H + L0        
*/

#ifdef __cplusplus
#include "the_matrix.hpp"
#endif

static int sizeof_L(int x)
{
    return (x+1) >> 1;
}

static int sizeof_H(int x)
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
        if (s == 0)
            H_p++;
        else
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
        if (s == 0)
            H_p++;
        else
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


#ifdef THE_MATRIX_DEFINED

static void dwt53_inplace(int*xy, int size)
{
   std::vector<int> tmp(size);
   dwt53(xy,size, tmp.data(), tmp.data() + sizeof_L(size));
   std::copy(tmp.begin(), tmp.end(), xy);
}

static void idwt53_inplace(int*xy, int size)
{
   std::vector<int> tmp(size);
   idwt53(tmp.data(),size, xy, xy + sizeof_L(size));
   std::copy(tmp.begin(), tmp.end(), xy);
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

static std::vector<int> calc_L_sizes( int size, int levels)
{
    std::vector<int> sizes(levels);
    sizes.clear();
    for (int level = 1; level <= levels && (size > 1); level++, size = sizeof_L(size))
    {
        sizes.push_back( size );
    }
    return sizes;
}

static void dwt53_lvl( std::vector<int> & row, int levels) 
{
    auto sizes = calc_L_sizes(row.size(), levels);    
    for (const auto s : sizes )
        dwt53_inplace( row.data(), s);
}

static void idwt53_lvl( std::vector<int> & row, int levels) 
{
    auto sizes = calc_L_sizes(row.size(),levels);

    for (auto it = sizes.rbegin(); it != sizes.rend(); ++it) {
        const auto s = *it;
        idwt53_inplace( row.data(), s );
    }
}


static void dwt53_rows(the_matrix &data, int levels)
{
    for (auto & row : data ) {        
        dwt53_lvl(row, levels);
    }
}

static void idwt53_rows(the_matrix &data, int levels)
{
    for (auto & row : data ) {        
        idwt53_lvl(row, levels);
    }
}


static void dwt53_2d(the_matrix & data, int levels) 
{
   //limit_levels
    levels = calc_L_sizes( std::min(data.size(), data[0].size()) , levels).size();
    if (levels > 0)
    {
      dwt53_rows(data, levels);
      transpose(data);
      dwt53_rows(data, levels);
    }

}

static void inv_dwt53_2d(the_matrix & data, int levels) 
{
   //limit_levels
    levels = calc_L_sizes( std::min(data.size(), data[0].size()), levels ).size();
    if (levels > 0)
    {
      idwt53_rows(data, levels);
      transpose(data);
      idwt53_rows(data, levels);
    }

}



namespace dwt2d {
struct Index {
    int offs;
    int size;
};

struct Block {
    Block() = default;
    Block(Index x, Index y) : x(x), y(y) {

    }
    Index x;
    Index y;
    void transpose() {
        std::swap(x,y);
    }

};

struct LevelBlock {
    Block LL_or_next_level;
    Block HL;
    Block LH;
    Block HH;
    void transpose() {
        LL_or_next_level.transpose();
        HL.transpose();
        LH.transpose();
        HH.transpose();
    }

};

enum H_type{
    LL, HL, LH, HH
};

}

static void get_dwt_sizes_rec(int &width, int &height, dwt2d::LevelBlock & lb)
{
    const auto L_width = sizeof_L(width);
    const auto H_width = sizeof_H(width);
    const auto L_height = sizeof_L(height);
    const auto H_height = sizeof_H(height);
    lb.LL_or_next_level.x.offs = 0;
    lb.LL_or_next_level.y.offs = 0;
    lb.LL_or_next_level.x.size = L_width;
    lb.LL_or_next_level.y.size = L_height;

    lb.HL.x.offs = L_width; 
    lb.HL.x.size = H_width;
    lb.HL.y.offs = 0;        
    lb.HL.y.size = L_height;

    lb.LH.x.offs = 0; 
    lb.LH.x.size = L_width;
    lb.LH.y.offs = L_height;        
    lb.LH.y.size = H_height;

    lb.HH.x.offs = L_width; 
    lb.HH.x.size = H_width;
    lb.HH.y.offs = L_height;        
    lb.HH.y.size = H_height;

    width = L_width;
    height = L_height;
        
}

static std::vector<dwt2d::LevelBlock> get_dwt_sizes(int width, int height, int levels, bool is_transposed )
{

    if ( is_transposed ) 
        std::swap(width, height);

    std::vector<dwt2d::LevelBlock> result;

    while (levels-- && width > 1 && height > 1) {
       
        auto & lb =  result.emplace_back();
        get_dwt_sizes_rec(width, height, lb  );
        if (is_transposed)
            lb.transpose();
   }
   return result;
}


template <typename F>
static void process_levels(the_matrix & a, int levels, bool is_transposed, F f)
{
    int width = a[0].size();
    int height = a.size();

    if ( is_transposed) 
        std::swap(width, height);

    dwt2d::LevelBlock lb;
    //int actual_levels = 0;
    for (int level=1; level<=levels && width > 1 && height > 1; ++level)
    {
        get_dwt_sizes_rec(width, height, lb);
        if (is_transposed)
            lb.transpose();
        f(level, dwt2d::HL, lb.HL);
        f(level, dwt2d::LH, lb.LH);
        f(level, dwt2d::HH, lb.HH);
        //actual_levels++;
    }
    
}


void compress_data(the_matrix & data, int levels=5, int Q=16, int details_level=2)
{
    dwt53_2d(data,levels); 
    //make sure dwt is transposed (for speed optimization reasons)
    process_levels(data,levels, true, [&data, Q,details_level](int level, dwt2d::H_type type, dwt2d::Block const& lb ){
        int q = Q >> ((level-1)*details_level); 
        if (type == dwt2d::HH)
            q<<=1;
        if (q <= 1)
            return;
        for (int y = lb.y.offs; y < lb.y.offs + lb.y.size; ++y)
        for (int x = lb.x.offs; x < lb.x.offs + lb.x.size; ++x)
        {
         
           data[y][x] = (( data[y][x] + q/2) / q);
        }
        //std::cerr << q << std::endl;

  });  
}

void decompress_data(the_matrix & data, int levels=5, int Q=16, int details_level=2)
{
    
    //make sure dwt is transposed (for speed optimization reasons)
    process_levels(data,levels, true, [&data, Q,details_level](int level, dwt2d::H_type type, dwt2d::Block const& lb ){
        int q = Q >> ((level-1)*details_level); 
        if (type == dwt2d::HH)
            q<<=1;
        if (q <= 1)
            return;
        for (int y = lb.y.offs; y < lb.y.offs + lb.y.size; ++y)
        for (int x = lb.x.offs; x < lb.x.offs + lb.x.size; ++x)
        {
         
           data[y][x] = data[y][x] *q;
        }
        //std::cerr << q << std::endl;

  });  
  inv_dwt53_2d(data,levels);
}


#endif