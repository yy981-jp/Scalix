# exec
file(GLOB_RECURSE SOURCES
	src/*.cpp
	../shared/*.cpp
)

set(EXE_NAME sl)
set(SDLIMAGE_DEPS_SHARED ON)

# Windows icon
if(WIN32)
	list(APPEND SOURCES
		${CMAKE_SOURCE_DIR}/cmake/icon.rc
	)
endif()

# protobuf
# generates engine/generated/*.pb.{cc,h} + tracker/generated/*_pb2.py*
generate_proto(pose)
generate_proto(instr)

list(APPEND SOURCES
	${pose_PROTO_SOURCES}
	${instr_PROTO_SOURCES}
)

add_executable(${EXE_NAME} ${SOURCES})

# shaders
# compiles engine/shaders/<name>/{vs,fs}.sc into build/runtime/*.bin
compile_shader(tex)
compile_shader(grid)

add_custom_target(${EXE_NAME}_shaders ALL
	DEPENDS
		${tex_SHADER_OUTPUTS}
		${grid_SHADER_OUTPUTS}
)

add_dependencies(${EXE_NAME} ${EXE_NAME}_shaders)

include(../cmake/cpack.cmake)


# external
find_package(yaml-cpp REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

# SDL
#
# external/install/debug or release should be added to CMAKE_PREFIX_PATH
# before reaching this file.
find_package(SDL3 CONFIG REQUIRED COMPONENTS SDL3)
find_package(SDL3_image CONFIG REQUIRED)

# bgfx
add_library(bgfx STATIC IMPORTED)
set_target_properties(bgfx PROPERTIES
	IMPORTED_LOCATION_DEBUG
		${CMAKE_SOURCE_DIR}/external/install/debug/lib/libbgfx.a
	IMPORTED_LOCATION_RELEASE
		${CMAKE_SOURCE_DIR}/external/install/release/lib/libbgfx.a

	INTERFACE_INCLUDE_DIRECTORIES
		${CMAKE_SOURCE_DIR}/external/install/release/include
)

# bx
add_library(bx STATIC IMPORTED)
set_target_properties(bx PROPERTIES
	IMPORTED_LOCATION_DEBUG
		${CMAKE_SOURCE_DIR}/external/install/debug/lib/libbx.a
	IMPORTED_LOCATION_RELEASE
		${CMAKE_SOURCE_DIR}/external/install/release/lib/libbx.a
)

# bimg
add_library(bimg STATIC IMPORTED)
set_target_properties(bimg PROPERTIES
	IMPORTED_LOCATION_DEBUG
		${CMAKE_SOURCE_DIR}/external/install/debug/lib/libbimg.a
	IMPORTED_LOCATION_RELEASE
		${CMAKE_SOURCE_DIR}/external/install/release/lib/libbimg.a
)


# compiler-specific fixes
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	set_source_files_properties(
		${CMAKE_SOURCE_DIR}/engine/src/gltf/tinygltf_impl.cpp
		PROPERTIES
			COMPILE_OPTIONS "-Wno-stringop-overflow"
	)
endif()


target_include_directories(${EXE_NAME} PRIVATE
	SYSTEM
		${CMAKE_SOURCE_DIR}/external
		${CMAKE_SOURCE_DIR}/external/nlohmann/include
		${CMAKE_SOURCE_DIR}/external/asio/include
		${CMAKE_SOURCE_DIR}/engine/src
		${CMAKE_SOURCE_DIR}/engine/generated
		${CMAKE_SOURCE_DIR}
)


target_link_libraries(${EXE_NAME} PRIVATE
	bgfx
	bx
	bimg

	yaml-cpp::yaml-cpp

	SDL3::SDL3
	SDL3_image::SDL3_image

	protobuf::libprotobuf
)


if(WIN32)
	target_compile_definitions(${EXE_NAME} PRIVATE
		_WIN32_WINNT=0x0A00
	)

	target_link_libraries(${EXE_NAME} PRIVATE
		ws2_32
	)
endif()


# mode
target_compile_options(${EXE_NAME} PRIVATE
	$<$<CONFIG:Release>:-O3>
	$<$<CONFIG:RelWithDebInfo>:-O3 -g>
)

target_compile_definitions(${EXE_NAME} PRIVATE
	$<$<CONFIG:Debug>:BX_CONFIG_DEBUG=1 BGFX_CONFIG_DEBUG=1>
	$<$<CONFIG:Release>:BX_CONFIG_DEBUG=0 BGFX_CONFIG_DEBUG=0>
	$<$<CONFIG:RelWithDebInfo>:BX_CONFIG_DEBUG=0 BGFX_CONFIG_DEBUG=0>
)

# message(FATAL_ERROR dll:${LOAD_WEBP_DYNAMIC})
