if(NOT EXISTS "${RUNTIME_EXE}")
	message(FATAL_ERROR "Executable not found: ${RUNTIME_EXE}")
endif()

set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM windows+pe)
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL objdump)

if(NOT CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND)
	find_program(
		CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND
		NAMES objdump
		REQUIRED
	)
endif()

file(GET_RUNTIME_DEPENDENCIES
	EXECUTABLES "${RUNTIME_EXE}"

	DIRECTORIES
		"${RUNTIME_BAZEL_BIN}"
		"${RUNTIME_DEBUG_BIN}"
		"${RUNTIME_RELEASE_BIN}"
		"${RUNTIME_MINGW_BIN}"

	RESOLVED_DEPENDENCIES_VAR runtime_deps
	UNRESOLVED_DEPENDENCIES_VAR unresolved_deps

	# Windows の API set DLL は配布対象から除外
	PRE_EXCLUDE_REGEXES
		"api-ms-.*"
		"ext-ms-.*"

	# Windows のシステム DLL は配布対象から除外
	POST_EXCLUDE_REGEXES
		".*[Ww][Ii][Nn][Dd][Oo][Ww][Ss]/.*"
		"C:\\WINDOWS\\system32/*"
)

foreach(dll IN LISTS runtime_deps)
	get_filename_component(dll_name "${dll}" NAME)
	get_filename_component(dll_dir "${dll}" DIRECTORY)

	set(destination "${RUNTIME_DEST}/${dll_name}")

	if(NOT "${dll_dir}" STREQUAL "${RUNTIME_DEST}")
		file(COPY_FILE
			"${dll}"
			"${destination}"
			ONLY_IF_DIFFERENT
		)
	endif()
endforeach()

if(unresolved_deps)
	message(WARNING
		"Unresolved runtime dependencies:\n${unresolved_deps}"
	)
endif()
