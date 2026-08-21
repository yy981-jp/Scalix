# exec
file(GLOB_RECURSE SOURCES src/*.cpp generated/*.c* ../shared/*.cpp)

# Windows icon
if (WIN32)
    list(APPEND SOURCES ${CMAKE_SOURCE_DIR}/cmake/icon.rc)
endif()

add_executable(${PROJECT_NAME} ${SOURCES})


# external
# add_subdirectory(${CMAKE_SOURCE_DIR}/external/yaml-cpp)


# bgfx
add_library(bgfx STATIC IMPORTED)
set_target_properties(bgfx PROPERTIES
    IMPORTED_LOCATION_DEBUG   ${CMAKE_SOURCE_DIR}/external/install/debug/lib/libbgfx.a
    IMPORTED_LOCATION_RELEASE ${CMAKE_SOURCE_DIR}/external/install/release/lib/libbgfx.a
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/external/install/release/include
)

add_library(bx STATIC IMPORTED)
set_target_properties(bx PROPERTIES
    IMPORTED_LOCATION_DEBUG   ${CMAKE_SOURCE_DIR}/external/install/debug/lib/libbx.a
    IMPORTED_LOCATION_RELEASE ${CMAKE_SOURCE_DIR}/external/install/release/lib/libbx.a
)

add_library(bimg STATIC IMPORTED)
set_target_properties(bimg PROPERTIES
    IMPORTED_LOCATION_DEBUG   ${CMAKE_SOURCE_DIR}/external/install/debug/lib/libbimg.a
    IMPORTED_LOCATION_RELEASE ${CMAKE_SOURCE_DIR}/external/install/release/lib/libbimg.a
)


# package
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
find_package(Protobuf CONFIG REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/external
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/external/nlohmann/include
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/engine/src
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/engine/generated
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    bgfx
    bx
    bimg
    yaml-cpp::yaml-cpp
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    protobuf::libprotobuf
)


if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _WIN32_WINNT=0x0A00 # win10,11
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ws2_32
    )
endif()


# mode
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:RelWithDebInfo>:-O3 -g>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:BX_CONFIG_DEBUG=1 BGFX_CONFIG_DEBUG=1>
    $<$<CONFIG:Release>:BX_CONFIG_DEBUG=0 BGFX_CONFIG_DEBUG=0>
    $<$<CONFIG:RelWithDebInfo>:BX_CONFIG_DEBUG=0 BGFX_CONFIG_DEBUG=0>
)
