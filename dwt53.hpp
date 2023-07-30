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


#ifdef __cplusplus
#ifdef THE_MATRIX_DEFINED
#include <vector>


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
    levels = calc_L_sizes( std::min(data.size(), data[0].size()) ).size();
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
    levels = calc_L_sizes( std::min(data.size(), data[0].size()) ).size();
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
    Index x;
    Index y;
};

struct LevelBlock {
    Block HL;
    Block LH;
    Block HH;
};
}

static std::vector<dwt2d::LevelBlock> get_dwt_sizes(the_matrix const& a, int levels)
{
    auto x_sizes = calc_L_sizes(a[0].size(),levels);
    auto y_sizes = calc_L_sizes(a.size(),levels);
    auto min_size = std::min( x_sizes.size(), y_sizes.size() );
    x_sizes.resize( min_size );
    y_sizes.resize( min_size );
    std::vector<dwt2d::LevelBlock> result(min_size);
    //fill sizes result
    for (int i=0; i<min_size; i++)
    {
        auto & lb = result[i];
        lb.HL.x.offs = sizeof_L(x_sizes[i]); 
        lb.HL.x.size = sizeof_H(x_sizes[i]);
        lb.HL.y.offs = 0;
        lb.HL.y.size = sizeof_H(y_sizes[i]);
        lb.HH.x.offs =  lb.HL.x.offs;
        lb.HH.y.offs =  lb.HL.y.size;
        lb.HH.x.size = sizeof_L(y_sizes[i]); 
        lb.HH.y.size = lb.HL.y.size;
        lb.LH.x.offs = 0;
        lb.LH.x.size = sizeof_L(x_sizes[i]);
        lb.LH.y.offs = lb.HH.y.offs;
        lb.LH.y.size = lb.HH.y.size;
    }
    return result;
}

static void scalar_quant_example(the_matrix & a, int levels, int Q) 
{
  
    auto sizes = get_dwt_sizes(data_copy,levels);
    for (auto const & si : sizes)
    {
        if (Q < 1)
            Q = 1;
        // std::cout << "HL:" << "x=" << si.HL.x.offs << ":" << si.HL.x.size <<  "  y=" << si.HL.y.offs << ":" << si.HL.y.size << std::endl;
        // std::cout << "LH:" << "x=" << si.LH.x.offs << ":" << si.LH.x.size <<  "  y=" << si.LH.y.offs << ":" << si.LH.y.size << std::endl;
        // std::cout << "HH:" << "x=" << si.HH.x.offs << ":" << si.HH.x.size <<  "  y=" << si.HH.y.offs << ":" << si.HH.y.size << std::endl;
        // std::cout << "----" << std::endl;
        for (int y=0; y < si.HH.y.size; ++y)
        for (int x=0; x < si.HH.x.size; ++x)
        {
            auto & coof_hh = data_copy[x + si.HH.x.offs][y + si.HH.y.offs];
            
            coof_hh = ((coof_hh + (Q)) / (Q*2)) * (Q*2);
        }
        for (int y=0; y < si.HL.y.size; ++y)
        for (int x=0; x < si.HL.x.size; ++x)
        {
            auto & coof_hl = data_copy[x + si.HL.x.offs][y + si.HL.y.offs];
            
            coof_hl = ((coof_hl + (Q/2)) / Q) * Q;
        }
        for (int y=0; y < si.LH.y.size; ++y)
        for (int x=0; x < si.LH.x.size; ++x)
        {
            auto & coof_lh = data_copy[x + si.LH.x.offs][y + si.LH.y.offs];
            
            coof_lh = ((coof_lh + (Q/2)) / Q) * Q;
        }
        Q /= 2;
  
    }
}
#endif
#endif
