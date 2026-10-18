cmake_policy(SET CMP0207 NEW)

include("${CMAKE_CURRENT_LIST_DIR}/runtime_dependencies.cmake")

foreach(_required_variable IN ITEMS RUNTIME_EXE RUNTIME_DEST)
	if(NOT DEFINED ${_required_variable} OR "${${_required_variable}}" STREQUAL "")
		message(FATAL_ERROR "Required variable is missing: ${_required_variable}")
	endif()
endforeach()

set(_runtime_search_directories)
foreach(_directory IN ITEMS
	"${RUNTIME_BAZEL_BIN}"
	"${RUNTIME_DEBUG_BIN}"
	"${RUNTIME_RELEASE_BIN}"
	"${RUNTIME_MINGW_BIN}"
)
	if(IS_DIRECTORY "${_directory}")
		list(APPEND _runtime_search_directories "${_directory}")
	endif()
endforeach()

# Include Bazel's direct output directories as well as its bazel-bin view.
get_filename_component(_bazel_external_directory "${RUNTIME_BAZEL_BIN}" DIRECTORY)
get_filename_component(_bazel_bin_directory "${_bazel_external_directory}" DIRECTORY)
get_filename_component(_bazel_workspace "${_bazel_bin_directory}" DIRECTORY)
file(GLOB _runtime_bazel_output_directories
	LIST_DIRECTORIES true
	"${_bazel_workspace}/bazel-out/*/bin/external/sxmp"
)
list(APPEND _runtime_search_directories ${_runtime_bazel_output_directories})

scalix_resolve_runtime_dependencies(
	EXECUTABLE "${RUNTIME_EXE}"
	DESTINATION "${RUNTIME_DEST}"
	PLATFORM "windows+pe"
	TOOL "objdump"
	COMMAND "${CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND}"
	SEARCH_DIRECTORIES ${_runtime_search_directories}
)
