# Verify that the license documents expected from the manifest exist after install.
if(NOT DEFINED LICENSE_INSTALL_ROOT OR LICENSE_INSTALL_ROOT STREQUAL "")
	message(FATAL_ERROR "LICENSE_INSTALL_ROOT must point to the temporary install prefix")
endif()
if(NOT DEFINED EXPECTED_FILES OR NOT EXISTS "${EXPECTED_FILES}")
	message(FATAL_ERROR "EXPECTED_FILES must point to the generated install manifest")
endif()

file(STRINGS "${EXPECTED_FILES}" _expected_files)
if(NOT _expected_files)
	message(FATAL_ERROR "The expected third-party license install list is empty")
endif()

set(_checked_count 0)
foreach(_relative_path IN LISTS _expected_files)
	if(_relative_path STREQUAL "")
		continue()
	endif()

	set(_installed_file "${LICENSE_INSTALL_ROOT}/${_relative_path}")
	if(NOT EXISTS "${_installed_file}" OR IS_DIRECTORY "${_installed_file}")
		message(FATAL_ERROR "Missing installed license document: ${_installed_file}")
	endif()
	math(EXPR _checked_count "${_checked_count} + 1")
endforeach()

message(STATUS "Verified ${_checked_count} installed license and notice files.")
