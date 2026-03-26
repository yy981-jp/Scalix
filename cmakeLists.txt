cmake_minimum_required(VERSION 3.16)
project(sl) # Scalix

set(CMAKE_CXX_STANDARD 23)
add_compile_options(-fdiagnostics-color=always)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# bgfx.cmake
set(BGFX_DIR ${CMAKE_SOURCE_DIR}/external/bgfx)
set(BX_DIR   ${CMAKE_SOURCE_DIR}/external/bx)
set(BIMG_DIR ${CMAKE_SOURCE_DIR}/external/bimg)
add_subdirectory(external/bgfx.cmake)

# exec
file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

# auto build command
add_custom_command(
	TARGET sl POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/runtime
	$<TARGET_FILE_DIR:sl>/runtime
)

# package
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

target_include_directories(${PROJECT_NAME} PRIVATE
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/external
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    bgfx
    bx
    bimg
    ${SDL2_LIBRARIES}
)

# mode
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:RelWithDebInfo>:-O3 -g>
)
