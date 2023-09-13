#pragma once

#define STB_IMAGE_IMPLEMENTATION
   #define STBI_NO_GIF
   #define STBI_NO_PSD
   #define STBI_NO_PIC
#include "stb_image.h"
#include <vector>
#include <stdexcept>
#include "the_matrix.hpp"

inline std::vector<the_matrix> load_image_as_yuv420(const char * img) {
    int width, height, num_channels;
    unsigned char* bitmap = stbi_load(img, &width, &height, &num_channels, 0);
    if (bitmap == NULL || width * height == 0 || num_channels == 0 || num_channels > 4)
    {
        throw std::logic_error("load image error!");
    }
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

