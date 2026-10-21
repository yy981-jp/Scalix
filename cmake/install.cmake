set(_scalix_runtime_dependency_module "${CMAKE_CURRENT_LIST_DIR}/runtime_dependencies.cmake")
include("${_scalix_runtime_dependency_module}")
get_filename_component(SCALIX_COMPILER_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

set(_scalix_runtime_search_directories
	"${EXTERNAL_INSTALL_PREFIX}/bin"
	"${EXTERNAL_INSTALL_PREFIX}/lib"
	"${EXTERNAL_INSTALL_PREFIX}/x64/mingw/bin"
	"${EXTERNAL_INSTALL_PREFIX}/x64/mingw/lib"
	"${SCALIX_COMPILER_BIN_DIR}"
	"${CMAKE_SOURCE_DIR}/sxmp/external/mediapipe/bazel-bin/external/sxmp"
)

# Bazel's actual output files can live in bazel-out rather than the bazel-bin
# convenience tree, especially for Windows external-repository outputs.
file(GLOB _scalix_sxmp_bazel_output_directories
	LIST_DIRECTORIES true
	"${CMAKE_SOURCE_DIR}/sxmp/external/mediapipe/bazel-out/*/bin/external/sxmp"
)
list(APPEND _scalix_runtime_search_directories ${_scalix_sxmp_bazel_output_directories})

set(_scalix_runtime_search_code "")
foreach(_directory IN LISTS _scalix_runtime_search_directories)
	string(APPEND _scalix_runtime_search_code "\n\t\t\t\"${_directory}\"")
endforeach()

# CMake selects the correct runtime dependency inspector for each platform.
# Explicitly pass objdump only on PE/ELF targets; Mach-O inspection uses otool.
set(_scalix_runtime_dependency_command_code "")
if(SCALIX_RUNTIME_DEPENDENCY_TOOL STREQUAL "objdump" AND CMAKE_OBJDUMP)
	string(APPEND _scalix_runtime_dependency_command_code
		"\n\t\tCOMMAND \"${CMAKE_OBJDUMP}\""
	)
endif()

install(
	TARGETS sl
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if(APPLE)
	# Bundle the staged MediaPipe backend beside sl before resolving Mach-O
	# runtime dependencies. Its install name is @rpath/libsxmp_be.dylib.
	install(
		FILES "${CMAKE_BINARY_DIR}/runtime-backend/libsxmp_be.dylib"
		DESTINATION "${CMAKE_INSTALL_BINDIR}"
	)
endif()

# install(
# 	DIRECTORY "${CMAKE_BINARY_DIR}/runtime/"
# 	DESTINATION "${CMAKE_INSTALL_BINDIR}/runtime"
# )

install(
	DIRECTORY "${CMAKE_SOURCE_DIR}/resources/"
	DESTINATION resources
)

install(CODE "
	include(\"${_scalix_runtime_dependency_module}\")
	scalix_resolve_runtime_dependencies(
		INSTALL_MODE
		STRICT
		EXECUTABLE \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/$<TARGET_FILE_NAME:sl>\"
		DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\"
		PLATFORM \"${SCALIX_RUNTIME_DEPENDENCY_PLATFORM}\"
		TOOL \"${SCALIX_RUNTIME_DEPENDENCY_TOOL}\"
		${_scalix_runtime_dependency_command_code}
		SEARCH_DIRECTORIES ${_scalix_runtime_search_code}
	)
")

include("${CMAKE_SOURCE_DIR}/cmake/third_party_licenses.cmake")
