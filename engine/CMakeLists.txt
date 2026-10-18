# exec
file(GLOB_RECURSE SOURCES
	src/*.cpp
	../shared/*.cpp
)

set(EXE_NAME sl)

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


# sxmp-backend is built by Bazel before CMake configuration.
set(SXMP_BACKEND_BAZEL_DIR
	"${CMAKE_SOURCE_DIR}/sxmp/external/mediapipe/bazel-bin/external/sxmp"
)

add_library(sxmp_be SHARED IMPORTED GLOBAL)
if(WIN32)
	set_target_properties(sxmp_be PROPERTIES
		IMPORTED_IMPLIB "${SXMP_BACKEND_BAZEL_DIR}/libsxmp_be.if.lib"
		IMPORTED_LOCATION "${SXMP_BACKEND_BAZEL_DIR}/libsxmp_be.dll"
	)
else()
	set_target_properties(sxmp_be PROPERTIES
		IMPORTED_LOCATION "${SXMP_BACKEND_BAZEL_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}sxmp_be${CMAKE_SHARED_LIBRARY_SUFFIX}"
	)

	if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		# Avoid embedding the Bazel output path in the executable's ELF dependencies.
		set_property(TARGET sxmp_be PROPERTY IMPORTED_NO_SONAME TRUE)
	endif()
endif()

if(WIN32)
	target_compile_definitions(${EXE_NAME} PRIVATE
		_WIN32_WINNT=0x0A00
	)

	target_link_libraries(${EXE_NAME} PRIVATE
		ws2_32
	)

	get_filename_component(SXMP_MINGW_BIN
		"${CMAKE_CXX_COMPILER}" DIRECTORY
	)

	set(SXMP_RUNTIME_OBJDUMP_ARG)
	if(CMAKE_OBJDUMP)
		list(APPEND SXMP_RUNTIME_OBJDUMP_ARG
			"-DCMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND=${CMAKE_OBJDUMP}"
		)
	endif()

	add_custom_target(${EXE_NAME}_runtime_deps
		COMMAND "${CMAKE_COMMAND}"
			"-DRUNTIME_EXE=$<TARGET_FILE:${EXE_NAME}>"
			"-DRUNTIME_DEST=$<TARGET_FILE_DIR:${EXE_NAME}>"
			"-DRUNTIME_BAZEL_BIN=${SXMP_BACKEND_BAZEL_DIR}"
			"-DRUNTIME_DEBUG_BIN=${CMAKE_SOURCE_DIR}/external/install/debug/x64/mingw/bin"
			"-DRUNTIME_RELEASE_BIN=${CMAKE_SOURCE_DIR}/external/install/release/x64/mingw/bin"
			"-DRUNTIME_MINGW_BIN=${SXMP_MINGW_BIN}"
			${SXMP_RUNTIME_OBJDUMP_ARG}
			-P "${CMAKE_SOURCE_DIR}/cmake/copy_runtime_deps.cmake"

		COMMAND "${CMAKE_COMMAND}" -E copy_if_different
			"${SXMP_BACKEND_BAZEL_DIR}/libsxmp_be.dll"
			"$<TARGET_FILE_DIR:${EXE_NAME}>/libsxmp_be.dll"

		VERBATIM
	)

	add_dependencies(${EXE_NAME}_runtime_deps ${EXE_NAME})
endif()


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


# External packages
find_package(bgfx CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core videoio)
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


target_compile_definitions(${EXE_NAME} PRIVATE
    TINYGLTF_NO_STB_IMAGE_WRITE
)


target_include_directories(${EXE_NAME} SYSTEM PRIVATE
	${CMAKE_SOURCE_DIR}/external
	${CMAKE_SOURCE_DIR}/external/nlohmann/include
	${CMAKE_SOURCE_DIR}/external/asio/include
	${CMAKE_SOURCE_DIR}/external/opencv/include
	${EXTERNAL_INSTALL_PREFIX}/include
	${EXTERNAL_INSTALL_PREFIX}/include/opencv5
)

target_include_directories(${EXE_NAME} PRIVATE
	${CMAKE_SOURCE_DIR}/engine/src
	${CMAKE_SOURCE_DIR}/engine/generated
	${CMAKE_SOURCE_DIR}
)

get_target_property(BGFX_LINK_LIBS bgfx::bgfx INTERFACE_LINK_LIBRARIES)

target_link_libraries(${EXE_NAME} PRIVATE
	bgfx::bgfx
	yaml-cpp::yaml-cpp
	SDL3::SDL3
	SDL3_image::SDL3_image
	${OpenCV_LIBS}

	protobuf::libprotobuf
	sxmp_be
)


# mode
target_compile_options(${EXE_NAME} PRIVATE
	$<$<CONFIG:Release>:-O3>
	$<$<CONFIG:RelWithDebInfo>:-O3 -g>
)
