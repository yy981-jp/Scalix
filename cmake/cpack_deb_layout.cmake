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
get_filename_component(_staging_root "${_install_prefix}" DIRECTORY)
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

# Runtime dependencies must be staged beside bin/sl before they are moved
# into the Debian private library directory. Do not recurse through the CPack
# work directory: that can accidentally collect the host's absolute path tree.
file(GLOB _runtime_libraries LIST_DIRECTORIES false "${_bin_directory}/*.so*")
if(NOT _runtime_libraries)
	message(FATAL_ERROR "No bundled ELF shared libraries found beside ${_scalix_executable}")
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

# dpkg-shlibdeps needs an explicit search directory for private libraries that
# were moved away from the executable's original staging directory.
# CPACK_PRE_BUILD_SCRIPTS runs in CPack's current makefile scope, so this value
# is available when the DEB generator computes automatic dependencies.
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS "${_runtime_library_directory}")

# Preserve the project license while also providing the Debian copyright file.
file(COPY "${_licenses_directory}/" DESTINATION "${_documentation_directory}")
if(EXISTS "${_documentation_directory}/LICENSE" AND NOT EXISTS "${_documentation_directory}/copyright")
	file(COPY_FILE "${_documentation_directory}/LICENSE" "${_documentation_directory}/copyright")
endif()
file(REMOVE_RECURSE "${_install_prefix}/share/licenses")

# The executable finds private libraries in /usr/lib/scalix. Every bundled
# library must use $ORIGIN too, not its original build-tree RUNPATH (which can
# contain Bazel _solib paths or the CI workspace's absolute directories).
find_program(_patchelf NAMES patchelf REQUIRED)
execute_process(
	COMMAND "${_patchelf}" --set-rpath "$ORIGIN/../lib/scalix" "${_scalix_executable}"
	RESULT_VARIABLE _patchelf_result
	OUTPUT_VARIABLE _patchelf_stdout
	ERROR_VARIABLE _patchelf_stderr
)
if(NOT _patchelf_result EQUAL 0)
	message(FATAL_ERROR
		"Could not set Debian executable RUNPATH: ${_patchelf_stdout}${_patchelf_stderr}"
	)
endif()

file(GLOB _runtime_library_files LIST_DIRECTORIES false "${_runtime_library_directory}/*.so*")
set(_patched_library_count 0)
foreach(_library IN LISTS _runtime_library_files)
	if(IS_SYMLINK "${_library}")
		continue()
	endif()
	execute_process(
		COMMAND "${_patchelf}" --set-rpath "$ORIGIN" "${_library}"
		RESULT_VARIABLE _patchelf_result
		OUTPUT_VARIABLE _patchelf_stdout
		ERROR_VARIABLE _patchelf_stderr
	)
	if(NOT _patchelf_result EQUAL 0)
		message(FATAL_ERROR
			"Could not set relocatable RUNPATH on ${_library}: "
			"${_patchelf_stdout}${_patchelf_stderr}"
		)
	endif()
	math(EXPR _patched_library_count "${_patched_library_count} + 1")
endforeach()

if(_patched_library_count EQUAL 0)
	message(FATAL_ERROR "No real ELF shared libraries were found in ${_runtime_library_directory}")
endif()

# Previous staging versions could create this accidental tree by applying
# DESTDIR twice in the custom install code. It is never part of the package.
if(IS_DIRECTORY "${_staging_root}/home")
	file(REMOVE_RECURSE "${_staging_root}/home")
endif()

if(NOT EXISTS "${_runtime_library_directory}/libsxmp_be.so")
	message(FATAL_ERROR "Debian staging is missing lib/scalix/libsxmp_be.so")
endif()
if(NOT EXISTS "${_install_prefix}/share/scalix/resources/runtime")
	message(FATAL_ERROR "Debian staging is missing share/scalix/resources/runtime")
endif()
message(STATUS
	"Prepared Debian/FHS layout under ${_install_prefix}; "
	"moved ${_runtime_libraries} runtime libraries into ${_runtime_library_directory}; "
	"rewrote RUNPATH on ${_patched_library_count} shared libraries"
)
