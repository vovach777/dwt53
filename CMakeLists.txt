cmake_minimum_required(VERSION 3.16)
project(dwt53 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
#if(CMAKE_BUILD_TYPE MATCHES Debug)
#    add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)
#    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#endif()
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_DEBUG} -Ofast")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" )

add_definitions(-D_USE_MATH_DEFINES)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)
add_executable(example_compress_decompress
    main.cpp
    bitstream.hpp
    cached_bitstream.hpp
    bitstream_helper.hpp
    dwt53.hpp
    huffman.hpp
    packmatrix.hpp
    quantization.hpp
    the_matrix.hpp
    utils.hpp
    dmc.hpp
    rangecoder.hpp
)

add_executable(dmc_compressor
    dmc_compressor.cpp
    dmc.hpp
    utils.hpp
    mio.hpp
)
