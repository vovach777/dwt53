cmake_minimum_required(VERSION 3.14)
project(dwt53 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")

add_executable(example_compress_decompress
    main.cpp
    bitstream.hpp
    dwt53.hpp               
    huffman.hpp             
    packmatrix.hpp          
    quantization.hpp        
    the_matrix.hpp          
    utils.hpp
)

