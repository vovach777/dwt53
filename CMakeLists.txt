cmake_minimum_required(VERSION 3.16)
project(dwt53 CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
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
#set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ppm8")


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
    stb_image.h
    img_load.hpp
    packmatrix.hpp
)

add_executable(dmc_compressor
    dmc_compressor.cpp
    dmc.hpp
    utils.hpp
    mio.hpp
    myargs/myargs.hpp
)

add_executable(range_compressor
    range_compressor.cpp
    rangecoder.hpp
    utils.hpp
    mio.hpp
    myargs/myargs.hpp
    )


add_executable(dmc_compressor_img
    dmc_compressor_img.cpp
    dmc.hpp
    utils.hpp
    mio.hpp
    myargs/myargs.hpp
    stb_image.h
    img_load.hpp
    the_matrix.hpp
    )

#find_library(Ppmd8_LIBRARY libPpmd8.a PATHS ${Ppmd8_LIBRARY_PATH})


#target_compile_definitions(dmc_compressor PRIVATE CMAKE_EXPORT_COMPILE_COMMANDS=1)


#add_executable(ppm8_compressor
#    ppm8_compressor.cpp
#    utils.hpp
#    mio.hpp
#    myargs/myargs.hpp
#    ppm8/Ppmd8cpp.hpp
#)

#target_link_libraries(ppm8_compressor  ${Ppmd8_LIBRARY})
