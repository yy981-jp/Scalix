# exec
file(GLOB_RECURSE SOURCES
	src/*.cpp
	../shared/*.cpp
)

set(EXE_NAME sl)
set(SDLIMAGE_DEPS_SHARED ON)

# External install prefix
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	list(PREPEND CMAKE_PREFIX_PATH
		${CMAKE_SOURCE_DIR}/external/install/debug
	)
else()
	list(PREPEND CMAKE_PREFIX_PATH
		${CMAKE_SOURCE_DIR}/external/install/release
	)
endif()


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


# External packages
find_package(bgfx CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(yaml-cpp REQUIRED)

if(WIN32 OR APPLE)
  find_package(Protobuf CONFIG REQUIRED)
else()
  find_package(Protobuf REQUIRED)
endif()


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

get_target_property(BGFX_LINK_LIBS bgfx::bgfx INTERFACE_LINK_LIBRARIES)

# message(STATUS "bgfx dependencies:")
# message(STATUS "${BGFX_LINK_LIBS}")

target_link_libraries(${EXE_NAME} PRIVATE
	bgfx::bgfx

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

# target_compile_definitions(${EXE_NAME} PRIVATE
# 	$<$<CONFIG:Debug>:BX_CONFIG_DEBUG=1 BGFX_CONFIG_DEBUG=1>
# 	$<$<CONFIG:Release>:BX_CONFIG_DEBUG=0 BGFX_CONFIG_DEBUG=0>
# 	$<$<CONFIG:RelWithDebInfo>:BX_CONFIG_DEBUG=0 BGFX_CONFIG_DEBUG=0>
# )
