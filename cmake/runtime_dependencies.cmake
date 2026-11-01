cmake_policy(SET CMP0207 NEW)

if(WIN32)
	set(SCALIX_RUNTIME_DEPENDENCY_PLATFORM "windows+pe")
	set(SCALIX_RUNTIME_DEPENDENCY_TOOL "objdump")
elseif(APPLE)
	set(SCALIX_RUNTIME_DEPENDENCY_PLATFORM "macos+macho")
	set(SCALIX_RUNTIME_DEPENDENCY_TOOL "otool")
elseif(UNIX)
	set(SCALIX_RUNTIME_DEPENDENCY_PLATFORM "linux+elf")
	set(SCALIX_RUNTIME_DEPENDENCY_TOOL "objdump")
else()
	message(FATAL_ERROR "Unsupported runtime dependency platform: ${CMAKE_SYSTEM_NAME}")
endif()

function(scalix_resolve_runtime_dependencies)
	cmake_parse_arguments(PARSE_ARGV 0 ARG
		"INSTALL_MODE;STRICT"
		"EXECUTABLE;DESTINATION;PLATFORM;TOOL;COMMAND"
		"SEARCH_DIRECTORIES;FALLBACK_DIRECTORIES;PRE_EXCLUDE_REGEXES;POST_EXCLUDE_REGEXES"
	)

	if(NOT ARG_EXECUTABLE OR NOT EXISTS "${ARG_EXECUTABLE}")
		message(FATAL_ERROR "Executable not found: ${ARG_EXECUTABLE}")
	endif()

	if(NOT ARG_DESTINATION)
		message(FATAL_ERROR "Runtime dependency destination was not provided")
	endif()

	if(NOT ARG_PLATFORM)
		set(ARG_PLATFORM "${SCALIX_RUNTIME_DEPENDENCY_PLATFORM}")
	endif()
	if(NOT ARG_TOOL)
		set(ARG_TOOL "${SCALIX_RUNTIME_DEPENDENCY_TOOL}")
	endif()

	set(_pre_exclude ${ARG_PRE_EXCLUDE_REGEXES})
	set(_post_exclude ${ARG_POST_EXCLUDE_REGEXES})

	if(NOT _pre_exclude)
		if(ARG_PLATFORM STREQUAL "windows+pe")
			set(_pre_exclude "api-ms-.*" "ext-ms-.*")
		elseif(ARG_PLATFORM STREQUAL "linux+elf")
			set(_pre_exclude
				"^linux-vdso\\.so.*"
				"^ld-linux.*\\.so.*"
				"^libc\\.so.*"
				"^libm\\.so.*"
				"^libpthread\\.so.*"
				"^libdl\\.so.*"
				"^librt\\.so.*"
				"^libresolv\\.so.*"
				"^libutil\\.so.*"
				"^libanl\\.so.*"
				"^libgcc_s\\.so.*"
				"^libstdc\\+\\+\\.so.*"
			)
		endif()
	endif()

	if(NOT _post_exclude)
		if(ARG_PLATFORM STREQUAL "windows+pe")
			set(_post_exclude ".*/[Ww][Ii][Nn][Dd][Oo][Ww][Ss]/.*")
		elseif(ARG_PLATFORM STREQUAL "macos+macho")
			set(_post_exclude "^/usr/lib/.*" "^/System/Library/.*")
		elseif(ARG_PLATFORM STREQUAL "linux+elf")
			# Do not exclude Linux dependencies by absolute directory. On usrmerge
			# systems (including Ubuntu), /lib may resolve to /usr/lib, and CMake
			# can report /lib/... paths for non-core libraries such as protobuf.
			# Filtering those paths would silently omit required runtime libraries.
			# Core system libraries are excluded by SONAME above instead.
		endif()
	endif()

	set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "${ARG_PLATFORM}")
	set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "${ARG_TOOL}")

	if(ARG_COMMAND)
		set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "${ARG_COMMAND}")
	elseif(ARG_TOOL STREQUAL "objdump" AND NOT CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND)
		find_program(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND NAMES objdump REQUIRED)
	endif()

	set(_search_directories)
	foreach(_directory IN LISTS ARG_SEARCH_DIRECTORIES)
		if(IS_DIRECTORY "${_directory}")
			list(APPEND _search_directories "${_directory}")
		endif()
	endforeach()
	set(_fallback_search_directories ${_search_directories})
	foreach(_directory IN LISTS ARG_FALLBACK_DIRECTORIES)
		if(IS_DIRECTORY "${_directory}")
			list(APPEND _fallback_search_directories "${_directory}")
		endif()
	endforeach()
	list(REMOVE_DUPLICATES _fallback_search_directories)

	if(ARG_PLATFORM STREQUAL "macos+macho")
		# Use BUNDLE_EXECUTABLE for a target built with MACOSX_BUNDLE. This lets
		# CMake inspect the executable in its app-bundle context on macOS.
		file(GET_RUNTIME_DEPENDENCIES
			EXECUTABLES "${ARG_EXECUTABLE}"
			BUNDLE_EXECUTABLE "${ARG_EXECUTABLE}"
			DIRECTORIES ${_search_directories}
			RESOLVED_DEPENDENCIES_VAR _resolved_dependencies
			UNRESOLVED_DEPENDENCIES_VAR _unresolved_dependencies
			PRE_EXCLUDE_REGEXES ${_pre_exclude}
			POST_EXCLUDE_REGEXES ${_post_exclude}
		)
	else()
		file(GET_RUNTIME_DEPENDENCIES
			EXECUTABLES "${ARG_EXECUTABLE}"
			DIRECTORIES ${_search_directories}
			RESOLVED_DEPENDENCIES_VAR _resolved_dependencies
			UNRESOLVED_DEPENDENCIES_VAR _unresolved_dependencies
			PRE_EXCLUDE_REGEXES ${_pre_exclude}
			POST_EXCLUDE_REGEXES ${_post_exclude}
		)
	endif()

	# CMake's platform resolver may leave a dependency unresolved even when its
	# file exists in one of the explicitly supplied search directories. Resolve
	# such names by exact filename, then inspect those files for their own
	# runtime dependencies. This keeps development copies and packages on the
	# same dependency-resolution path.
	set(_resolved_dependency_names)
	foreach(_dependency IN LISTS _resolved_dependencies)
		get_filename_component(_dependency_name "${_dependency}" NAME)
		if(ARG_PLATFORM STREQUAL "windows+pe")
			string(TOLOWER "${_dependency_name}" _dependency_name)
		endif()
		list(APPEND _resolved_dependency_names "${_dependency_name}")
	endforeach()

	while(_unresolved_dependencies)
		set(_remaining_unresolved)
		set(_fallback_dependencies)

		foreach(_dependency IN LISTS _unresolved_dependencies)
			# Mach-O install names commonly use @rpath/, @loader_path/, or
			# @executable_path/ prefixes. Search directories contain the actual
			# library files, so use only the basename when probing those paths.
			get_filename_component(_dependency_file "${_dependency}" NAME)
			set(_dependency_name "${_dependency_file}")
			if(ARG_PLATFORM STREQUAL "windows+pe")
				string(TOLOWER "${_dependency_name}" _dependency_name)
			endif()

			if(_dependency_name IN_LIST _resolved_dependency_names)
				continue()
			endif()

			# Prefer explicit paths before find_file(). On Windows/MinGW this
			# avoids relying on CMake's find search behavior for runtime DLLs
			# installed by our external projects.
			unset(_fallback_dependency)
			foreach(_directory IN LISTS _fallback_search_directories)
				set(_candidate "${_directory}/${_dependency_file}")
				if(EXISTS "${_candidate}" AND NOT IS_DIRECTORY "${_candidate}")
					set(_fallback_dependency "${_candidate}")
					break()
				endif()
			endforeach()

			if(NOT _fallback_dependency)
				find_file(_fallback_dependency
					NAMES "${_dependency_file}"
					PATHS ${_fallback_search_directories}
					NO_DEFAULT_PATH
					NO_CACHE
				)
			endif()

			if(_fallback_dependency)
				list(APPEND _fallback_dependencies "${_fallback_dependency}")
				list(APPEND _resolved_dependencies "${_fallback_dependency}")
				list(APPEND _resolved_dependency_names "${_dependency_name}")
			else()
				list(APPEND _remaining_unresolved "${_dependency}")
			endif()
			unset(_fallback_dependency)
		endforeach()

		if(NOT _fallback_dependencies)
			set(_unresolved_dependencies ${_remaining_unresolved})
			break()
		endif()

		list(REMOVE_DUPLICATES _fallback_dependencies)
		if(ARG_PLATFORM STREQUAL "macos+macho")
			file(GET_RUNTIME_DEPENDENCIES
				LIBRARIES ${_fallback_dependencies}
				BUNDLE_EXECUTABLE "${ARG_EXECUTABLE}"
				DIRECTORIES ${_search_directories}
				RESOLVED_DEPENDENCIES_VAR _additional_resolved_dependencies
				UNRESOLVED_DEPENDENCIES_VAR _additional_unresolved_dependencies
				PRE_EXCLUDE_REGEXES ${_pre_exclude}
				POST_EXCLUDE_REGEXES ${_post_exclude}
			)
		else()
			file(GET_RUNTIME_DEPENDENCIES
				LIBRARIES ${_fallback_dependencies}
				DIRECTORIES ${_search_directories}
				RESOLVED_DEPENDENCIES_VAR _additional_resolved_dependencies
				UNRESOLVED_DEPENDENCIES_VAR _additional_unresolved_dependencies
				PRE_EXCLUDE_REGEXES ${_pre_exclude}
				POST_EXCLUDE_REGEXES ${_post_exclude}
			)
		endif()

		foreach(_dependency IN LISTS _additional_resolved_dependencies)
			get_filename_component(_dependency_name "${_dependency}" NAME)
			if(ARG_PLATFORM STREQUAL "windows+pe")
				string(TOLOWER "${_dependency_name}" _dependency_name)
			endif()
			list(APPEND _resolved_dependency_names "${_dependency_name}")
		endforeach()

		list(APPEND _resolved_dependencies ${_additional_resolved_dependencies})
		list(REMOVE_DUPLICATES _resolved_dependencies)
		list(REMOVE_DUPLICATES _resolved_dependency_names)

		list(APPEND _remaining_unresolved ${_additional_unresolved_dependencies})
		list(REMOVE_DUPLICATES _remaining_unresolved)
		set(_unresolved_dependencies ${_remaining_unresolved})
	endwhile()

	if(_unresolved_dependencies)
		if(ARG_STRICT)
			message(FATAL_ERROR
				"Unresolved runtime dependencies:\n${_unresolved_dependencies}\n"
				"Searched directories:\n${_search_directories}"
			)
		else()
			message(WARNING
				"Unresolved runtime dependencies:\n${_unresolved_dependencies}\n"
				"Searched directories:\n${_search_directories}"
			)
		endif()
	endif()

	if(NOT _resolved_dependencies)
		message(STATUS "No additional runtime dependencies found for ${ARG_EXECUTABLE}")
		return()
	endif()

	if(ARG_INSTALL_MODE)
		file(INSTALL
			DESTINATION "${ARG_DESTINATION}"
			TYPE SHARED_LIBRARY
			FOLLOW_SYMLINK_CHAIN
			FILES ${_resolved_dependencies}
		)

		if(ARG_PLATFORM STREQUAL "linux+elf")
			# The runtime libraries are installed next to the executable.
			# Rewrite any build-tree RUNPATHs and add a local search path to
			# every real ELF shared library (not its symlink aliases).
			find_program(_patchelf NAMES patchelf REQUIRED)
			set(_linux_runtime_files "${ARG_EXECUTABLE}")
			file(GLOB _linux_runtime_libraries
				LIST_DIRECTORIES false
				"${ARG_DESTINATION}/*.so*"
			)
			foreach(_runtime_file IN LISTS _linux_runtime_libraries)
				if(NOT IS_SYMLINK "${_runtime_file}")
					list(APPEND _linux_runtime_files "${_runtime_file}")
				endif()
			endforeach()
			list(REMOVE_DUPLICATES _linux_runtime_files)

			foreach(_runtime_file IN LISTS _linux_runtime_files)
				execute_process(
					COMMAND "${_patchelf}" --set-rpath "$ORIGIN" "${_runtime_file}"
					RESULT_VARIABLE _patchelf_result
					OUTPUT_VARIABLE _patchelf_stdout
					ERROR_VARIABLE _patchelf_stderr
				)
				if(NOT _patchelf_result EQUAL 0)
					message(FATAL_ERROR
						"Could not set relocatable ELF RUNPATH on ${_runtime_file}: "
						"${_patchelf_stdout}${_patchelf_stderr}"
					)
				endif()
			endforeach()
		elseif(ARG_PLATFORM STREQUAL "macos+macho")
			# CMake copies Mach-O dependencies but does not rewrite absolute
			# Homebrew install names. Make the app bundle self-contained:
			# - the executable uses its @rpath entry for Frameworks;
			# - dylibs use @loader_path because all bundled dylibs share a dir.
			find_program(_otool NAMES otool REQUIRED)
			find_program(_install_name_tool NAMES install_name_tool REQUIRED)

			set(_macos_runtime_files "${ARG_EXECUTABLE}")
			foreach(_dependency IN LISTS _resolved_dependencies)
				get_filename_component(_dependency_name "${_dependency}" NAME)
				set(_installed_dependency "${ARG_DESTINATION}/${_dependency_name}")
				if(EXISTS "${_installed_dependency}" AND NOT IS_DIRECTORY "${_installed_dependency}")
					list(APPEND _macos_runtime_files "${_installed_dependency}")
				endif()
			endforeach()
			list(REMOVE_DUPLICATES _macos_runtime_files)

			foreach(_binary IN LISTS _macos_runtime_files)
				execute_process(
					COMMAND "${_otool}" -L "${_binary}"
					RESULT_VARIABLE _otool_result
					OUTPUT_VARIABLE _otool_output
					ERROR_VARIABLE _otool_stderr
				)
				if(NOT _otool_result EQUAL 0)
					message(FATAL_ERROR "Could not inspect Mach-O dependencies for ${_binary}: ${_otool_stderr}")
				endif()

				string(REPLACE "\r" "" _otool_output "${_otool_output}")
				string(REPLACE "\n" ";" _otool_lines "${_otool_output}")
				foreach(_line IN LISTS _otool_lines)
					string(STRIP "${_line}" _line)
					if(NOT _line MATCHES "^(.+) \\(compatibility version")
						continue()
					endif()

					set(_load_name "${CMAKE_MATCH_1}")
					if(NOT IS_ABSOLUTE "${_load_name}")
						continue()
					endif()
					if(_load_name MATCHES "^/usr/lib/" OR _load_name MATCHES "^/System/Library/")
						continue()
					endif()

					get_filename_component(_load_basename "${_load_name}" NAME)
					set(_bundled_library "${ARG_DESTINATION}/${_load_basename}")
					if(NOT EXISTS "${_bundled_library}")
						continue()
					endif()

					if("${_binary}" STREQUAL "${ARG_EXECUTABLE}")
						set(_new_load_name "@rpath/${_load_basename}")
					else()
						set(_new_load_name "@loader_path/${_load_basename}")
					endif()

					if(NOT "${_load_name}" STREQUAL "${_new_load_name}")
						execute_process(
							COMMAND "${_install_name_tool}" -change "${_load_name}" "${_new_load_name}" "${_binary}"
							RESULT_VARIABLE _install_name_result
							OUTPUT_VARIABLE _install_name_stdout
							ERROR_VARIABLE _install_name_stderr
						)
						if(NOT _install_name_result EQUAL 0)
							message(FATAL_ERROR
								"Could not relocate Mach-O dependency ${_load_name} in ${_binary}: "
								"${_install_name_stdout}${_install_name_stderr}"
							)
						endif()
					endif()
				endforeach()
			endforeach()
		endif()
	else()
		file(MAKE_DIRECTORY "${ARG_DESTINATION}")
		foreach(_dependency IN LISTS _resolved_dependencies)
			get_filename_component(_dependency_name "${_dependency}" NAME)
			set(_destination "${ARG_DESTINATION}/${_dependency_name}")

			if(EXISTS "${_destination}")
				file(REAL_PATH "${_dependency}" _dependency_real_path)
				file(REAL_PATH "${_destination}" _destination_real_path)
				if(_dependency_real_path STREQUAL _destination_real_path)
					continue()
				endif()
			endif()

			file(COPY_FILE "${_dependency}" "${_destination}" ONLY_IF_DIFFERENT)
		endforeach()
	endif()
endfunction()
