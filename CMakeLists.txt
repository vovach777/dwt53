cmake_minimum_required(VERSION 3.16)
#set(CMAKE_TOOLCHAIN_FILE D:/Sources/C++/Projects/clang/vcpkg/scripts/buildsystems/vcpkg.cmake)
project(dwt53 CXX)
message(STATUS "CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}")
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

#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" )
#set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ppm8")
#target_compile_options(dmc_compressor_img PUBLIC "-Wno-shift-op-parentheses")

add_definitions(-D_USE_MATH_DEFINES)
#set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)
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


add_executable(dmc_img
    dmc_compressor_img.cpp
    dmc.hpp
    utils.hpp
    mio.hpp
    myargs/myargs.hpp
    stb_image.h
    stb_image_write.h
    img_load.hpp
    the_matrix.hpp
    timed.hpp
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

add_executable(rle8_compressor_img
    rle8_compressor_img.cpp
    quantization.hpp
    utils.hpp
    mio.hpp
    myargs/myargs.hpp
    stb_image.h
    img_load.hpp
    the_matrix.hpp
    )

add_executable(snd_comp
    snd_comp.cpp
    resampler.cpp
    resampler.hpp
    myargs/myargs.hpp
    the_matrix.hpp
    dwt53.hpp
    dmc.hpp
    )

#target_compile_definitions(snd_comp PRIVATE )
#target_compile_options(snd_comp PRIVATE -mfma -ffast-math)
target_compile_options(snd_comp PRIVATE -ffast-math -msse2)
find_library(ALGLIB_LIBRARY alglib.lib  PATHS  "${CMAKE_CURRENT_SOURCE_DIR}/alglib-cpp/src/build" )
target_link_libraries(snd_comp ${ALGLIB_LIBRARY})
target_include_directories(snd_comp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/alglib-cpp/src")



# find_package(FFMPEG REQUIRED)

# target_include_directories(snd_comp PRIVATE ${FFMPEG_INCLUDE_DIRS})
# target_link_directories(snd_comp PRIVATE ${FFMPEG_LIBRARY_DIRS})
# target_link_libraries(snd_comp PRIVATE ${FFMPEG_LIBRARIES})

add_executable(snd_comp_huff
    snd_comp_huff.cpp
    myargs/myargs.hpp
    huffman.hpp
    mio.hpp
    utils.hpp
    timed.hpp
    bitstream.hpp
    )

add_executable(snd_comp_dmc
    snd_comp_dmc.cpp
    myargs/myargs.hpp
    dmc.hpp
    mio.hpp
    utils.hpp
    timed.hpp
    )

add_executable(snd_comp_fht
    snd_comp_fht.cpp
    myargs/myargs.hpp
    dmc.hpp
    mio.hpp
    utils.hpp
    timed.hpp
    )