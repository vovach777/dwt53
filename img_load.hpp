#pragma once

#define STB_IMAGE_IMPLEMENTATION
   #define STBI_NO_GIF
   #define STBI_NO_PSD
   #define STBI_NO_PIC
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <vector>
#include <stdexcept>
#include "the_matrix.hpp"
#include "timed.hpp"

inline stbi_uc *stbi_load_timed(char const *filename, int *x, int *y, int *comp, int req_comp)
{
    profiling::timed stb_load("load image (stb)");
    return stbi_load(filename, x,y,comp,req_comp);
}

inline std::vector<the_matrix> load_image_as_yuv420(const char * img) {
    int width, height, num_channels;
    unsigned char* bitmap = stbi_load_timed(img, &width, &height, &num_channels, 0);
    if (bitmap == NULL || width * height == 0 || num_channels == 0 || num_channels > 4)
    {
        throw std::logic_error("load image error!");
    }
    profiling::timed convert_to_yuv420("component loader");
    std::vector<the_matrix> result;
    switch (num_channels)  {
        case 2:
        case 1: {
            result.emplace_back(height, std::vector<int>(width));
            if ( num_channels == 2) {
                result.emplace_back(height, std::vector<int>(width));
            }
            uint8_t* gray8_p = bitmap;
            for (int h=0; h<height; h++)
            for (int x=0; x<width; x++)
            {
                result[0][h][x] = *gray8_p++;
                if (num_channels == 2) {
                    result[1][h][x] = *gray8_p++;
                }
            }
            }
            break;
        case 3:
        case 4:
            {
                uint8_t* rgb_p = bitmap;
                result.emplace_back(height, std::vector<int>(width));
                result.emplace_back((height+1)>>1, std::vector<int>((width+1)>>1));
                result.emplace_back((height+1)>>1, std::vector<int>((width+1)>>1));
                if (num_channels == 4)
                    result.emplace_back(height, std::vector<int>(width));
                int stride = num_channels * width;

                for (int h=0; h<height; h++)
                for (int x=0; x<width; x++)
                {
                    //luma
                    result[0][h][x] = +0.29900f*rgb_p[0] + 0.58700f*rgb_p[1] + 0.11400f*rgb_p[2];
                    if (num_channels == 4)
                        result[3][h][x] = rgb_p[3];
                    //subsampling
                    if ( ( (h|x)&1 ) == 0 ) {
                        auto rgb01_p = x+1 < width  ? rgb_p+num_channels : rgb_p;
                        auto rgb10_p = h+1 < height ? rgb_p+stride : rgb_p;
                        auto rgb11_p = x+1 < width && h+1 < height ? rgb_p+stride+num_channels : rgb_p;
                        int r = (rgb_p[0] + rgb01_p[0] + rgb10_p[0] + rgb11_p[0]) / 4;
                        int g = (rgb_p[1] + rgb01_p[1] + rgb10_p[1] + rgb11_p[1]) / 4;
                        int b = (rgb_p[2] + rgb01_p[2] + rgb10_p[2] + rgb11_p[2]) / 4;
                        //chroma
                        result[1][h>>1][x>>1] = -0.16874f*r - 0.33126f*g + 0.50000f*b + 128;
                        result[2][h>>1][x>>1] = +0.50000f*r - 0.41869f*g - 0.08131f*b + 128;
                    }
                    rgb_p += num_channels;
                }
            }
    }
    free(bitmap);
    return result;
}

#ifndef stbi__float2fixed
#define stbi__float2fixed(x)  (((int) ((x) * 4096.0f + 0.5f)) << 8)
#endif
template <typename T>
static void YCbCr_to_RGB(int y, int cb, int cr,  T& r_, T& g_, T& b_)
{
      int y_fixed = (y << 20) + (1<<19); // rounding
      int r,g,b;
      cr -= 128;
      cb -= 128;
      r = y_fixed +  cr* stbi__float2fixed(1.40200f);
      g = y_fixed + (cr*-stbi__float2fixed(0.71414f)) + ((cb*-stbi__float2fixed(0.34414f)) & 0xffff0000);
      b = y_fixed                                     +   cb* stbi__float2fixed(1.77200f);
      r >>= 20;
      g >>= 20;
      b >>= 20;
      if ((unsigned) r > 255) { if (r < 0) r = 0; else r = 255; }
      if ((unsigned) g > 255) { if (g < 0) g = 0; else g = 255; }
      if ((unsigned) b > 255) { if (b < 0) b = 0; else b = 255; }
      r_ = r;
      g_ = g;
      b_ = b;
}


static void save_rgb(const char * img, uint8_t* rgb, int width, int height, int stride, int nb_channel)
{
   size_t i = strlen(img)-1;
   if (i > 3) {
      if (
         tolower(img[i-2]) == 'p' &&
         tolower(img[i-1]) == 'n' &&
         tolower(img[i])   == 'g') {
         stbi_write_png(img, width,height, nb_channel, rgb, stride);
         return;
      };
      if (
         tolower(img[i-2]) == 'j' &&
         tolower(img[i-1]) == 'p' &&
         tolower(img[i])   == 'g') {
         stbi_write_jpg (img, width,height, nb_channel, rgb, 100);
         return;
      };
      if (
         tolower(img[i-2]) == 't' &&
         tolower(img[i-1]) == 'g' &&
         tolower(img[i])   == 'a') {
         stbi_write_tga(img, width,height, nb_channel, rgb);
         return;
      };
   }
   stbi_write_bmp(img,width,height,nb_channel,rgb);
}


inline void store_image_as_yuv420(const std::vector<the_matrix> planes, const char * img) {

    int planes_nb = planes.size();
    auto height = planes[0].size();
    auto width = planes[0][0].size();
    std::vector<uint8_t> rgb( height * width * planes_nb );
    auto stride = width*planes_nb;
    auto chroma_h_log2 = planes_nb < 3 ? 0 : (planes[1].size() == height ? 0 : (  (planes[1].size() == (height+1>>1)) ? 1 : (planes[1].size() == (height+1>>2)) ? 2 : 3  ));
    auto chroma_w_log2 = planes_nb < 3 ? 0 : (planes[1][0].size() == width ? 0 : (  (planes[1][0].size() == (width+1>>1)) ? 1 : (planes[1][0].size() == (width+1>>2)) ? 2 : 3  ));
    //std::cout << "yuv -> rgb" << " " << width << "x" << height <<  std::endl;
    auto rgb_p = rgb.data();
    for (int h=0; h < height; ++h)
    for (int x=0; x < width;  ++x)
    {
        //auto rgb_p = &rgb[h*stride+x*3];
        if (planes_nb < 3) {
            *rgb_p++ =  planes[0][ h ][ x ];
            if (planes_nb == 2)
                *rgb_p++ =  planes[1][ h ][ x ];
        } else {
            auto y = planes[0][ h ][ x ];
            auto u = planes[1][h>>chroma_h_log2][x>>chroma_w_log2];
            auto v = planes[2][h>>chroma_h_log2][x>>chroma_w_log2];
            YCbCr_to_RGB(y,u,v, rgb_p[0],rgb_p[1],rgb_p[2]);
            if (planes_nb == 4)
                rgb_p[3] = planes[3][ h ][ x ];
            rgb_p += planes_nb;
        }

    }

    save_rgb(img,rgb.data(), width, height, stride, planes_nb);
}