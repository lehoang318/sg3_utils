cmake_minimum_required(VERSION 3.20)

project(sg3_utils)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
add_compile_definitions(HAVE_CONFIG_H _LARGEFILE64_SOURCE _FILE_OFFSET_BITS=64)
add_compile_options(-Wall -W -g -O2)

add_subdirectory(lib)
add_subdirectory(examples)
