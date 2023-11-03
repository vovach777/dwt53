#include <iostream>
#include <cmath>

float linear(float x, float width, float x0, float x1)
{

    return (x1-x0) / width * x + x0;

}


int main() {

            for (float x = -4.0f; x < 2.5f +  (6.5f/8.0f) ; x += (6.5f/8.0f) ) {

                std::cout << int(expf(x) * (32768.0 / 12.5f)) -48 << ", ";

            }




}