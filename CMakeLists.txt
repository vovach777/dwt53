cmake_minimum_required(VERSION 3.14)

project(dwt53 CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(example_compress_decompress
    main.cpp
    packmatrix.cpp
    )
