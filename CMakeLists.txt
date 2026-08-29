cmake_minimum_required(VERSION 3.21)
project(
	scalix
	VERSION 0.1.0
	DESCRIPTION "High-performance 3D engine"
	LANGUAGES CXX
)

cmake_policy(SET CMP0207 NEW)

set(CMAKE_CXX_STANDARD 23)
add_compile_options(-fdiagnostics-color=always)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

include_directories(
	${CMAKE_SOURCE_DIR}
)

# CMake modules
include(cmake/protobuf.cmake)
include(cmake/shaders.cmake)

# external
# add_subdirectory(${CMAKE_SOURCE_DIR}/external/yaml-cpp)

include(GNUInstallDirs)

# Scalix
add_subdirectory(engine)
add_subdirectory(importer)
add_subdirectory(dev)
