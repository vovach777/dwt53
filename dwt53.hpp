#pragma once

#ifdef __cplusplus
#define __inline inline
#else
#define __inline static
#endif
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
        LHLHLHL => H-variants:  LHL (C);   L-variants:  xLH (A), HLH (D), HLx
  (B)

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

__inline int sizeof_L(int x) { return (x + 1) >> 1; }

__inline int sizeof_H(int x) { return x >> 1; }

__inline void dwt53(const int *x, int size, int *L, int *H) {
    if (size & 1) {
        // C-variant
        int *H_p = H;
        const int *x_p = x + 1;
        int s = size / 2;
        while (s--) {
            *H_p++ = x_p[0] - (x_p[-1] + x_p[1]) / 2;
            x_p += 2;
        }
        int *L_p = L;
        // A-variant
        *L_p++ = x[0] + (H[0] + 1) / 2;
        // D-variant
        s = (size + 1) / 2 - 2;
        x_p = x + 2;
        H_p = H;
        if (s == 0)
            H_p++;
        else
            while (s--) {
                *L_p++ = x_p[0] + (H_p[0] + H_p[1] + 2) / 4;
                H_p += 1;
                x_p += 2;
            }
        // B-variant
        *L_p++ = x_p[0] + (H_p[-1] + 1) / 2;

    } else {
        // C-variant
        int s = size / 2 - 1;
        int *H_p = H;
        const int *x_p = x + 1;
        while (s--) {
            *H_p++ = x_p[0] - (x_p[-1] + x_p[1]) / 2;
            x_p += 2;
        }
        // E-variant
        *H_p = x_p[0] - x_p[-1];
        s = size / 2 - 1;
        int *L_p = L;
        H_p = H;
        // A-variant
        *L_p++ = x[0] + (H_p[0] + 1) / 2;
        // D-variant
        x_p = x + 2;
        while (s--) {
            *L_p++ = x_p[0] + (H_p[0] + H_p[1] + 2) / 4;
            H_p += 1;
            x_p += 2;
        }
    }
}

__inline void idwt53(int *x, int size, const int *L, const int *H) {
    if (size & 1) {
        const int *L_p = L;
        // A-variant
        //*L_p++ = x[0]+(H[0] + 1)/2;
        x[0] = *L_p++ - (H[0] + 1) / 2;
        // D-variant
        int s = (size + 1) / 2 - 2;
        int *x_p = x + 2;
        const int *H_p = H;
        if (s == 0)
            H_p++;
        else
            while (s--) {
                //*L_p++ = x_p[0] + (H_p[0] + H_p[1] + 2) / 4;
                x_p[0] = *L_p++ - (H_p[0] + H_p[1] + 2) / 4;
                H_p += 1;
                x_p += 2;
            }
        // B-variant
        //*L_p++ = x_p[0]+(H_p[-1] + 1) /2;
        x_p[0] = *L_p++ - (H_p[-1] + 1) / 2;

        // C-variant
        H_p = H;
        x_p = x + 1;
        s = size / 2;
        while (s--) {
            //*H_p++ = x_p[0] - (x_p[-1] + x_p[1])/2;
            x_p[0] = *H_p++ + (x_p[-1] + x_p[1]) / 2;
            x_p += 2;
        }
    } else {
        // LLLLLL
        //////
        int s = size / 2 - 1;
        const int *L_p = L;
        const int *H_p = H;
        // A-variant
        x[0] = *L_p++ - (H_p[0] + 1) / 2;
        // D-variant
        int *x_p = x + 2;
        while (s--) {
            x_p[0] = *L_p++ - (H_p[0] + H_p[1] + 2) / 4;
            H_p += 1;
            x_p += 2;
        }
        // HHHHHHH
        // C-variant
        s = size / 2 - 1;
        H_p = H;
        x_p = x + 1;
        while (s--) {
            x_p[0] = *H_p++ + (x_p[-1] + x_p[1]) / 2;
            x_p += 2;
        }
        // E-variant
        x_p[0] = *H_p + x_p[-1];
    }
}

__inline void haar(const int *x, int size, int *L, int *H) {
    int *H_p = H;
    int *L_p = L;
    const int *x_p = x;
    int s = size / 2;
    while (s--) {
        int a = x_p[0];
        int b = x_p[1];
        b -= a;
        a += b / 2;
        *L_p++ = a;
        *H_p++ = b;
        x_p += 2;
    }
    if (size & 1) {
        *L_p++ = x_p[0];
    }
}

__inline void ihaar(int *x, int size, const int *L, const int *H) {
    const int *H_p = H;
    const int *L_p = L;
    int *x_p = x;
    int s = size / 2;
    while (s--) {
        int a = *L_p++;
        int b = *H_p++;
        a -= b / 2;
        b += a;
        *x_p++ = a;
        *x_p++ = b;
    }
    if (size & 1) {
        *x_p = *L_p;
    }
}

#ifndef no_dwt2d
#ifdef __cplusplus
#include "the_matrix.hpp"
#endif

#ifdef THE_MATRIX_DEFINED
#include "quantization.hpp"

namespace dwt2d {

using DWTFunction = decltype(dwt53) *;
using IDWTFunction = decltype(idwt53) *;

enum Wavelet { unchanged, haar, dwt53 };

enum BlockType { HL, LH, HH };

struct Band {
    int nb;
    int width;
    int height;
    int widthLL;
    int heightLL;
    Block blocks[3];
    Band(int num, int &width, int &height)
        : nb(num), width(width), height(height) {
        const auto L_width = sizeof_L(width);
        const auto H_width = sizeof_H(width);
        const auto L_height = sizeof_L(height);
        const auto H_height = sizeof_H(height);
        widthLL = L_width;
        heightLL = L_height;

        blocks[HL].x.offs = L_width;
        blocks[HL].x.size = H_width;
        blocks[HL].y.offs = 0;
        blocks[HL].y.size = L_height;

        blocks[LH].x.offs = 0;
        blocks[LH].x.size = L_width;
        blocks[LH].y.offs = L_height;
        blocks[LH].y.size = H_height;

        blocks[HH].x.offs = L_width;
        blocks[HH].x.size = H_width;
        blocks[HH].y.offs = L_height;
        blocks[HH].y.size = H_height;

        width = L_width;
        height = L_height;
    }
};

inline std::vector<dwt2d::Band> calc_geometry(int width, int height,
                                              int levels) {
    std::vector<Band> result;
    int num = 1;
    while (levels-- && width > 1 && height > 1) {
        auto &lb = result.emplace_back(num++, width, height);
    }
    return result;
}

template <typename F>
inline void process_levels(the_matrix &a, int levels, F f) {
    int width = a[0].size();
    int height = a.size();

    for (int level = 1; level <= levels && width > 1 && height > 1; ++level) {
        dwt2d::Band band(level, width, height);
        f(band);
    }
}

class Transform {
    the_matrix data_;
    DWTFunction forward_ = &::dwt53;
    IDWTFunction inverse_ = &::idwt53;
    int levels_ = 1;
    std::vector<int> LH_storage_;
    std::vector<dwt2d::Band> geometry_;
   public:
   Transform() = default;
    void set_transform(DWTFunction forward, IDWTFunction inverse) {
        forward_ = forward;
        inverse_ = inverse;
    }
    void set_transform(Wavelet transform) {
        if (transform == haar) {
            forward_ = &::haar;
            inverse_ = &::ihaar;
        } else if (transform == dwt53) {
            forward_ = &::dwt53;
            inverse_ = &::idwt53;
        }
    }

    the_matrix &get_data() { return data_; }

    // static std::vector<int> calc_L_sizes( int size, int levels)
    // {
    //     std::vector<int> sizes(levels);
    //     sizes.clear();
    //     for (int level = 1; level <= levels && (size > 1); level++, size =
    //     sizeof_L(size))
    //     {
    //         sizes.push_back( size );
    //     }
    //     return sizes;
    // }

    void prepare_transform(int levels, Wavelet transform,
                           the_matrix const &data) {
        data_ = data;
        set_transform(transform);
        LH_storage_.resize(std::max DUMMY (data[0].size(), data.size()));
        geometry_ = calc_geometry(data[0].size(), data.size(), levels);
        levels_ = geometry_.size();
    }

    the_matrix& prepare_transform(int levels, Wavelet transform, size_t width, size_t height) {
        set_transform(transform);
        LH_storage_.resize(std::max DUMMY (width, height));
        geometry_ = calc_geometry(width, height, levels);
        levels_ = geometry_.size();
        data_ = the_matrix(height,std::vector<int>(width));
        return data_;
    }


    the_matrix &forward() {
        for (auto &row : data_) {
            for (int i = 0; i < geometry_.size(); i++) {
                const auto size = geometry_[i].width;
                forward_(row.data(), size, LH_storage_.data(),
                         LH_storage_.data() + sizeof_L(size));
                std::copy(LH_storage_.begin(), LH_storage_.begin() + size,
                          row.begin());
            }
        }
        for (int x = 0; x < data_[0].size(); x++) {
            auto col = get_col(data_, x);
            for (int i = 0; i < geometry_.size(); i++) {
                const auto size = geometry_[i].height;
                forward_(col.data(), size, LH_storage_.data(),
                         LH_storage_.data() + sizeof_L(size));
                std::copy(LH_storage_.begin(), LH_storage_.begin() + size,
                          col.begin());
            }
            set_col(data_, x, col);
        }
        return data_;
    }

    the_matrix &inverse() {
        for (int x = 0; x < data_[0].size(); x++) {
            auto col = get_col(data_, x);
            for (int i = geometry_.size() - 1; i >= 0; --i) {
                const auto size = geometry_[i].height;
                std::copy(col.begin(), col.begin() + size, LH_storage_.begin());
                inverse_(col.data(), size, LH_storage_.data(),
                         LH_storage_.data() + sizeof_L(size));
            }
            set_col(data_, x, col);
        }

        for (auto &row : data_) {
            for (int i = geometry_.size() - 1; i >= 0; --i) {
                const auto size = geometry_[i].width;
                std::copy(row.begin(), row.begin() + size, LH_storage_.begin());
                inverse_(row.data(), size, LH_storage_.data(),
                         LH_storage_.data() + sizeof_L(size));
            }
        }
        return data_;
    }

    void quantization(int Q = 4, int details=4) {


        //TODO: менять фактор Q в зависимости от уровня.

        scalar_adaptive_quantization lockup_table;
        assert( lockup_table.is_enabled() == false);

        int max_value = 0;

        //Quant only high components
        for (int pass = 1; pass <= 2; ++pass)
        for (int level = 0; level < levels_; ++level) {
            for (int i = 0; i < 3; i++) //except HH
                process_matrix(data_, geometry_[level].blocks[i],
                               [&](int v) {
                                    if (pass == 1)
                                        max_value = std::max DUMMY ( max_value, std::abs(v));
                                    else {
                                        if ( ! lockup_table.is_enabled() )
                                            lockup_table.init(Q,max_value, details);
                                        else
                                            lockup_table.update_histogram(std::abs(v));
                                    }
                               });
        }


        for (int level = 0; level < levels_; ++level) {
            for (int i = 0; i < 3; i++)
                process_matrix(
                    data_, geometry_[level].blocks[i],
                    [&](int &v) { v = lockup_table[v];}
                    );
        }
    }

};

}  // namespace dwt2d

#endif
#endif