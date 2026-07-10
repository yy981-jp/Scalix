set(sxim "sxim") # Scalix impoter

# exec
file(GLOB_RECURSE SOURCES src/*.cpp ../shared/*.cpp)
add_executable(${sxim} ${SOURCES})

# package
find_package(yaml-cpp REQUIRED)

target_include_directories(${sxim} PRIVATE
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/external
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/importer/src
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}
)

target_link_libraries(${sxim} PRIVATE
    yaml-cpp::yaml-cpp
)

# mode
target_compile_options(${sxim} PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:RelWithDebInfo>:-O3 -g>
)
