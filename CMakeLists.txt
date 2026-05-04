cmake_minimum_required(VERSION 3.16)
project(sl) # Scalix

set(CMAKE_CXX_STANDARD 23)
add_compile_options(-fdiagnostics-color=always)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# bgfx.cmake
set(BGFX_DIR ${CMAKE_SOURCE_DIR}/external/bgfx)
set(BX_DIR   ${CMAKE_SOURCE_DIR}/external/bx)
set(BIMG_DIR ${CMAKE_SOURCE_DIR}/external/bimg)
add_subdirectory(external/bgfx.cmake)

# Scalix
add_subdirectory(engine)
add_subdirectory(importer)
