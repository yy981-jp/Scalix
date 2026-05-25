cmake_minimum_required(VERSION 3.16)
project(sl) # Scalix

set(CMAKE_CXX_STANDARD 23)
add_compile_options(-fdiagnostics-color=always)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# Scalix
add_subdirectory(engine)
add_subdirectory(importer)
add_subdirectory(dev)
