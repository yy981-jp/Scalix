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


# Generated platform icons. CI creates these from assets/src/icon.svg before
# CMake configuration; keep generated binaries out of the source repository.
set(SCALIX_GENERATED_ASSET_DIR "${CMAKE_SOURCE_DIR}/assets/dist/assets")
if(WIN32)
	set(SCALIX_WINDOWS_ICON "${SCALIX_GENERATED_ASSET_DIR}/icon.ico")
	if(NOT EXISTS "${SCALIX_WINDOWS_ICON}")
		message(FATAL_ERROR "Generated Windows icon not found: ${SCALIX_WINDOWS_ICON}")
	endif()

	configure_file(
		"${CMAKE_SOURCE_DIR}/cmake/icon.rc"
		"${CMAKE_CURRENT_BINARY_DIR}/icon.rc"
		@ONLY
	)
	list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/icon.rc")
elseif(APPLE)
	set(SCALIX_MACOS_ICON "${SCALIX_GENERATED_ASSET_DIR}/icon.icns")
	if(NOT EXISTS "${SCALIX_MACOS_ICON}")
		message(FATAL_ERROR "Generated macOS icon not found: ${SCALIX_MACOS_ICON}")
	endif()

	set_source_files_properties("${SCALIX_MACOS_ICON}" PROPERTIES
		MACOSX_PACKAGE_LOCATION "Resources"
	)
	list(APPEND SOURCES "${SCALIX_MACOS_ICON}")
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

if(APPLE)
	# Build a conventional Scalix.app bundle while keeping the executable name
	# stable for scripts and runtime diagnostics.
	set_target_properties(${EXE_NAME} PROPERTIES
		MACOSX_BUNDLE TRUE
		OUTPUT_NAME "Scalix"
		MACOSX_BUNDLE_BUNDLE_NAME "Scalix"
		MACOSX_BUNDLE_GUI_IDENTIFIER "jp.yy981.scalix"
		MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
		MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
		MACOSX_BUNDLE_ICON_FILE "icon.icns"
		INSTALL_RPATH "@executable_path/../Frameworks"
	)
endif()


# sxmp-backend is built by Bazel before CMake configuration.
set(SXMP_BACKEND_BAZEL_DIR
	"${CMAKE_SOURCE_DIR}/sxmp/external/mediapipe/bazel-bin/external/sxmp"
)

if(APPLE)
	set(SXMP_BACKEND_RUNTIME_LIBRARY
		"${CMAKE_BINARY_DIR}/runtime-backend/libsxmp_be.dylib"
	)
else()
	set(SXMP_BACKEND_RUNTIME_LIBRARY
		"${SXMP_BACKEND_BAZEL_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}sxmp_be${CMAKE_SHARED_LIBRARY_SUFFIX}"
	)
endif()

add_library(sxmp_be SHARED IMPORTED GLOBAL)
if(WIN32)
	set_target_properties(sxmp_be PROPERTIES
		IMPORTED_IMPLIB "${SXMP_BACKEND_BAZEL_DIR}/libsxmp_be.if.lib"
		IMPORTED_LOCATION "${SXMP_BACKEND_BAZEL_DIR}/libsxmp_be.dll"
	)
else()
	set_target_properties(sxmp_be PROPERTIES
		IMPORTED_LOCATION "${SXMP_BACKEND_RUNTIME_LIBRARY}"
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

	# Track runtime DLL inputs so Ninja can skip dependency scanning when the
	# executable, backend, resolver scripts, and candidate DLLs are unchanged.
	set(_runtime_dependency_inputs
		"${CMAKE_CURRENT_LIST_FILE}"
		"${CMAKE_SOURCE_DIR}/cmake/copy_runtime_deps.cmake"
		"${CMAKE_SOURCE_DIR}/cmake/runtime_dependencies.cmake"
		"${SXMP_BACKEND_BAZEL_DIR}/libsxmp_be.dll"
		"${SXMP_BACKEND_BAZEL_DIR}/libsxmp_be.if.lib"
	)
	set(_runtime_dependency_directories
		"${SXMP_BACKEND_BAZEL_DIR}"
		"${CMAKE_SOURCE_DIR}/external/install/debug/bin"
		"${CMAKE_SOURCE_DIR}/external/install/debug/x64/mingw/bin"
		"${CMAKE_SOURCE_DIR}/external/install/release/bin"
		"${CMAKE_SOURCE_DIR}/external/install/release/x64/mingw/bin"
		"${SXMP_MINGW_BIN}"
	)

	# Bazel may place runtime dependencies in a configuration-specific output
	# directory, in addition to bazel-bin's stable view.
	get_filename_component(_bazel_external_directory "${SXMP_BACKEND_BAZEL_DIR}" DIRECTORY)
	get_filename_component(_bazel_bin_directory "${_bazel_external_directory}" DIRECTORY)
	get_filename_component(_bazel_workspace "${_bazel_bin_directory}" DIRECTORY)
	file(GLOB _runtime_bazel_output_directories
		LIST_DIRECTORIES true
		CONFIGURE_DEPENDS
		"${_bazel_workspace}/bazel-out/*/bin/external/sxmp"
	)
	list(APPEND _runtime_dependency_directories ${_runtime_bazel_output_directories})

	foreach(_directory IN LISTS _runtime_dependency_directories)
		if(IS_DIRECTORY "${_directory}")
			file(GLOB _runtime_dlls
				CONFIGURE_DEPENDS
				"${_directory}/*.dll"
				"${_directory}/*.DLL"
			)
			list(APPEND _runtime_dependency_inputs ${_runtime_dlls})
		endif()
	endforeach()
	list(REMOVE_DUPLICATES _runtime_dependency_inputs)

	# Keep a stable manifest whose contents change only when the input file set
	# changes. GLOB CONFIGURE_DEPENDS also makes Ninja notice added/removed DLLs.
	set(SXMP_RUNTIME_DEPENDENCY_MANIFEST
		"${CMAKE_CURRENT_BINARY_DIR}/${EXE_NAME}_runtime_dependency_inputs.txt"
	)
	string(JOIN "\n" _runtime_dependency_manifest_content ${_runtime_dependency_inputs})
	file(CONFIGURE
		OUTPUT "${SXMP_RUNTIME_DEPENDENCY_MANIFEST}"
		CONTENT "${_runtime_dependency_manifest_content}\n"
	)

	set(SXMP_RUNTIME_DEPS_STAMP
		"${CMAKE_CURRENT_BINARY_DIR}/${EXE_NAME}_runtime_deps.stamp"
	)
	add_custom_command(
		OUTPUT "${SXMP_RUNTIME_DEPS_STAMP}"
		BYPRODUCTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libsxmp_be.dll"
		COMMAND "${CMAKE_COMMAND}"
			"-DRUNTIME_EXE=$<TARGET_FILE:${EXE_NAME}>"
			"-DRUNTIME_DEST=$<TARGET_FILE_DIR:${EXE_NAME}>"
			"-DRUNTIME_BAZEL_BIN=${SXMP_BACKEND_BAZEL_DIR}"
			"-DRUNTIME_DEBUG_PACKAGE_BIN=${CMAKE_SOURCE_DIR}/external/install/debug/bin"
			"-DRUNTIME_DEBUG_BIN=${CMAKE_SOURCE_DIR}/external/install/debug/x64/mingw/bin"
			"-DRUNTIME_RELEASE_PACKAGE_BIN=${CMAKE_SOURCE_DIR}/external/install/release/bin"
			"-DRUNTIME_RELEASE_BIN=${CMAKE_SOURCE_DIR}/external/install/release/x64/mingw/bin"
			"-DRUNTIME_MINGW_BIN=${SXMP_MINGW_BIN}"
			${SXMP_RUNTIME_OBJDUMP_ARG}
			-P "${CMAKE_SOURCE_DIR}/cmake/copy_runtime_deps.cmake"

		COMMAND "${CMAKE_COMMAND}" -E copy_if_different
			"${SXMP_BACKEND_BAZEL_DIR}/libsxmp_be.dll"
			"$<TARGET_FILE_DIR:${EXE_NAME}>/libsxmp_be.dll"

		COMMAND "${CMAKE_COMMAND}" -E touch
			"${SXMP_RUNTIME_DEPS_STAMP}"

		DEPENDS
			"${SXMP_RUNTIME_DEPENDENCY_MANIFEST}"
			${_runtime_dependency_inputs}

		VERBATIM
	)

	add_custom_target(${EXE_NAME}_runtime_deps ALL
		DEPENDS "${SXMP_RUNTIME_DEPS_STAMP}"
	)
	add_dependencies(${EXE_NAME}_runtime_deps ${EXE_NAME})
endif()


# shaders
# compiles engine/shaders/<name>/{vs,fs}.sc
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
	${CMAKE_SOURCE_DIR}/external/CLI11/include
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
	scalix_rmlui_bgfx
)


# mode
target_compile_options(${EXE_NAME} PRIVATE
	$<$<CONFIG:Release>:-O3>
	$<$<CONFIG:RelWithDebInfo>:-O3 -g>
)
