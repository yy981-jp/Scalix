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

scalix_resolve_runtime_dependencies(
	EXECUTABLE "${RUNTIME_EXE}"
	DESTINATION "${RUNTIME_DEST}"
	PLATFORM "windows+pe"
	TOOL "objdump"
	COMMAND "${CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND}"
	SEARCH_DIRECTORIES ${_runtime_search_directories}
)
