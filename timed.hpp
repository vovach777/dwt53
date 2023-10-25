#pragma once
#include <chrono>
#include <iomanip>
#include <iostream>

namespace profiling {
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;

struct timed {
  std::string name;
  high_resolution_clock::time_point start;
  timed() : timed("") {}
  timed( const std::string & name ) : name(name), start(high_resolution_clock::now())
  {}
  ~timed(){
    auto s = duration<double>(high_resolution_clock::now() - start).count();
    std::cout << name << " execution time: " << std::fixed << std::setprecision(9) <<  s << std::endl;
  }
};
}