cmake_minimum_required(VERSION 3.16)
project(bgfx_superbuild)

include(ExternalProject)

# Debug
ExternalProject_Add(bgfx_debug
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/bgfx.cmake
	BINARY_DIR ${CMAKE_SOURCE_DIR}/build_debug

	CMAKE_ARGS
		-DCMAKE_BUILD_TYPE=Debug
		-DCMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/install/debug
		-DBGFX_DIR=${CMAKE_SOURCE_DIR}/bgfx
		-DBX_DIR=${CMAKE_SOURCE_DIR}/bx
		-DBIMG_DIR=${CMAKE_SOURCE_DIR}/bimg

	INSTALL_COMMAND ${CMAKE_COMMAND} --install .
)
# Release
ExternalProject_Add(bgfx_release
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/bgfx.cmake
	BINARY_DIR ${CMAKE_SOURCE_DIR}/build_release

	CMAKE_ARGS
		-DCMAKE_BUILD_TYPE=Release
		-DCMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/install/release
		-DBGFX_DIR=${CMAKE_SOURCE_DIR}/bgfx
		-DBX_DIR=${CMAKE_SOURCE_DIR}/bx
		-DBIMG_DIR=${CMAKE_SOURCE_DIR}/bimg

	INSTALL_COMMAND ${CMAKE_COMMAND} --install .
)
