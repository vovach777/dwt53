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

static void transpose(the_matrix & data) {
    // this assumes that all inner vectors have the same size and
    // allocates space for the complete result in advance
    the_matrix result(data[0].size(), std::vector<int>(data.size()));
    for (int i = 0; i < data[0].size(); i++) 
        for (int j = 0; j < data.size(); j++) {
            result[i][j] = data[j][i];
        }
    data = result;
}

static void dwt53_rows(the_matrix &data, int levels)
{
    for (auto & row : data ) {        
        for (int level = 1;  level <= levels; level++)
        {
                auto size =  row.size() >> (level-1);
                if (size > 1) {
                    dwt53_inplace( row.data(), size );
                }
                
        }
    }
}

static void inv_dwt53_rows(the_matrix &data, int levels)
{
    for (auto & row : data ) {        
        for (int level = levels; level > 0; level--)
        {
            auto size =  row.size() >> (level-1);
            if (size > 1) {
                idwt53_inplace( row.data(), size );
            }
        }
    }
}


static void dwt53_2d(the_matrix & data, int levels) 
{
    dwt53_rows(data, levels);
    transpose(data);
    dwt53_rows(data, levels);

}

static void inv_dwt53_2d(the_matrix & data, int levels) 
{
    inv_dwt53_rows(data, levels);
    transpose(data);
    inv_dwt53_rows(data, levels);

}

#endif
#endif
