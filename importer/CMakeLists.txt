set(sxim "sxim") # Scalix impoter

# exec
file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(${sxim} ${SOURCES})

# package
find_package(PkgConfig REQUIRED)

target_include_directories(${sxim} PRIVATE
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/external
)

target_link_libraries(${sxim} PRIVATE
)

# mode
target_compile_options(${sxim} PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:RelWithDebInfo>:-O3 -g>
)
