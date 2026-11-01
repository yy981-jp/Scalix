# CPack runs this after installing files into its temporary staging tree.
# This script is selected only for the DEB generator.

if(DEFINED CPACK_TEMPORARY_INSTALL_DIRECTORY AND IS_DIRECTORY "${CPACK_TEMPORARY_INSTALL_DIRECTORY}")
	set(_stage_root "${CPACK_TEMPORARY_INSTALL_DIRECTORY}")
elseif(DEFINED CPACK_TEMPORARY_DIRECTORY AND IS_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}")
	set(_stage_root "${CPACK_TEMPORARY_DIRECTORY}")
else()
	message(FATAL_ERROR "CPack did not expose its temporary install directory")
endif()

# Find the staged prefix by locating the installed sl executable. This also
# tolerates an intermediate directory added by CPack.
file(GLOB_RECURSE _staged_files LIST_DIRECTORIES false "${_stage_root}/*")
set(_scalix_executable "")
foreach(_candidate IN LISTS _staged_files)
	get_filename_component(_candidate_name "${_candidate}" NAME)
	if(NOT _candidate_name STREQUAL "sl")
		continue()
	endif()
	get_filename_component(_candidate_directory "${_candidate}" DIRECTORY)
	get_filename_component(_candidate_directory_name "${_candidate_directory}" NAME)
	if(_candidate_directory_name STREQUAL "bin")
		if(_scalix_executable)
			message(FATAL_ERROR "Found multiple staged Scalix executables: ${_scalix_executable}; ${_candidate}")
		endif()
		set(_scalix_executable "${_candidate}")
	endif()
endforeach()

if(NOT _scalix_executable)
	message(FATAL_ERROR "Could not locate staged bin/sl under ${_stage_root}")
endif()

get_filename_component(_bin_directory "${_scalix_executable}" DIRECTORY)
get_filename_component(_install_prefix "${_bin_directory}" DIRECTORY)
set(_resources_directory "${_install_prefix}/resources")
set(_licenses_directory "${_install_prefix}/share/licenses/Scalix")
set(_documentation_directory "${_install_prefix}/share/doc/scalix")
set(_runtime_library_directory "${_install_prefix}/lib/scalix")

if(NOT IS_DIRECTORY "${_resources_directory}")
	message(FATAL_ERROR "Staged resources directory is missing: ${_resources_directory}")
endif()
if(NOT IS_DIRECTORY "${_licenses_directory}")
	message(FATAL_ERROR "Staged license directory is missing: ${_licenses_directory}")
endif()

file(MAKE_DIRECTORY
	"${_runtime_library_directory}"
	"${_install_prefix}/share/scalix"
	"${_documentation_directory}"
)
file(RENAME "${_resources_directory}" "${_install_prefix}/share/scalix/resources")

# Runtime dependency installation can use a different staging path depending
# on the CPack generator. Collect every bundled ELF shared library from the
# package staging tree instead of assuming it was placed beside bin/sl.
file(GLOB_RECURSE _staged_files LIST_DIRECTORIES false "${_stage_root}/*")
set(_runtime_libraries)
foreach(_candidate IN LISTS _staged_files)
	get_filename_component(_candidate_name "${_candidate}" NAME)
	if(_candidate_name MATCHES "\\.so($|\\.)")
		string(FIND "${_candidate}" "${_runtime_library_directory}/" _already_in_runtime_directory)
		if(_already_in_runtime_directory EQUAL 0)
			continue()
		endif()
		list(APPEND _runtime_libraries "${_candidate}")
	endif()
endforeach()

if(NOT _runtime_libraries)
	message(FATAL_ERROR
		"No bundled ELF shared libraries were staged under ${_stage_root}. "
		"Expected runtime dependencies beside ${_scalix_executable} or elsewhere in the CPack staging tree."
	)
endif()

foreach(_library IN LISTS _runtime_libraries)
	get_filename_component(_library_name "${_library}" NAME)
	set(_library_destination "${_runtime_library_directory}/${_library_name}")
	if(EXISTS "${_library_destination}" OR IS_SYMLINK "${_library_destination}")
		message(FATAL_ERROR
			"Duplicate staged runtime library name '${_library_name}': "
			"${_library} and ${_library_destination}"
		)
	endif()
	file(RENAME "${_library}" "${_library_destination}")
endforeach()

# Preserve the project license while also providing the Debian copyright file.
file(COPY "${_licenses_directory}/" DESTINATION "${_documentation_directory}")
if(EXISTS "${_documentation_directory}/LICENSE" AND NOT EXISTS "${_documentation_directory}/copyright")
	file(COPY_FILE "${_documentation_directory}/LICENSE" "${_documentation_directory}/copyright")
endif()
file(REMOVE_RECURSE "${_install_prefix}/share/licenses")

# The executable finds its private libraries in /usr/lib/scalix.
execute_process(
	COMMAND patchelf --set-rpath "$ORIGIN/../lib/scalix" "${_scalix_executable}"
	RESULT_VARIABLE _patchelf_result
	OUTPUT_VARIABLE _patchelf_stdout
	ERROR_VARIABLE _patchelf_stderr
)
if(NOT _patchelf_result EQUAL 0)
	message(FATAL_ERROR
		"Could not set Debian executable RUNPATH: ${_patchelf_stdout}${_patchelf_stderr}"
	)
endif()

if(NOT EXISTS "${_runtime_library_directory}/libsxmp_be.so")
	message(FATAL_ERROR "Debian staging is missing lib/scalix/libsxmp_be.so")
endif()
if(NOT EXISTS "${_install_prefix}/share/scalix/resources/runtime")
	message(FATAL_ERROR "Debian staging is missing share/scalix/resources/runtime")
endif()
message(STATUS
	"Prepared Debian/FHS layout under ${_install_prefix}; "
	"moved ${_runtime_libraries} runtime libraries into ${_runtime_library_directory}"
)
