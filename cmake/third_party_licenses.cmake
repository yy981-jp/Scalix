# Generate third-party notices and install license documents from the CSV manifest.
#
# THIRD_PARTY_LICENSES.csv is the source of truth. The parser intentionally supports
# a simple CSV subset: four comma-separated fields, no embedded commas or newlines,
# and optional double quotes around the path field only.

set(_scalix_license_manifest "${CMAKE_SOURCE_DIR}/THIRD_PARTY_LICENSES.csv")
if(NOT EXISTS "${_scalix_license_manifest}")
	message(FATAL_ERROR "Third-party license manifest not found: ${_scalix_license_manifest}")
endif()

file(STRINGS "${_scalix_license_manifest}" _scalix_license_rows ENCODING UTF-8)

set(_scalix_license_components)
if(APPLE)
	set(_scalix_license_install_root "Scalix.app/Contents/Resources/licenses/Scalix")
else()
	set(_scalix_license_install_root "${CMAKE_INSTALL_DATADIR}/licenses/Scalix")
endif()

set(_scalix_license_expected_destinations
	"${_scalix_license_install_root}/LICENSE"
	"${_scalix_license_install_root}/THIRD_PARTY_NOTICES.md"
)
set(_scalix_license_seen_destinations)

foreach(_row IN LISTS _scalix_license_rows)
	string(STRIP "${_row}" _row)

	if(_row STREQUAL "" OR _row MATCHES "^#")
		continue()
	endif()

	if(_row STREQUAL "component,license,source_url,path")
		continue()
	endif()

	string(REGEX MATCH [[^([^,]+),([^,]+),([^,]+),("[^"]+"|[^,]+)$]] _matched "${_row}")
	if(NOT _matched)
		message(FATAL_ERROR
			"Invalid third-party license CSV row: ${_row}\n"
			"Expected: component,license,source_url,path"
		)
	endif()

	set(_component "${CMAKE_MATCH_1}")
	set(_license "${CMAKE_MATCH_2}")
	set(_source_url "${CMAKE_MATCH_3}")
	set(_path_field "${CMAKE_MATCH_4}")

	if(_path_field MATCHES [[^"([^"]+)"$]])
		set(_path "${CMAKE_MATCH_1}")
	else()
		set(_path "${_path_field}")
	endif()

	string(STRIP "${_component}" _component)
	string(STRIP "${_license}" _license)
	string(STRIP "${_source_url}" _source_url)
	string(STRIP "${_path}" _path)

	if(NOT _component MATCHES "^[A-Za-z0-9._+-]+$")
		message(FATAL_ERROR "Invalid component name in license CSV: ${_component}")
	endif()
	if(NOT _license MATCHES "^[A-Za-z0-9.+-]+$")
		message(FATAL_ERROR "Invalid SPDX license identifier for ${_component}: ${_license}")
	endif()
	if(NOT _source_url MATCHES [[^https://[^ |<>]+$]])
		message(FATAL_ERROR "Invalid source URL for ${_component}: ${_source_url}")
	endif()
	if(_path STREQUAL "" OR _path MATCHES [[(^|/)\.\.(/|$)]])
		message(FATAL_ERROR "Invalid relative license path for ${_component}: ${_path}")
	endif()

	cmake_path(IS_ABSOLUTE _path _path_is_absolute)
	if(_path_is_absolute)
		message(FATAL_ERROR "License path must be relative to the repository: ${_path}")
	endif()

	set(_source_file "${CMAKE_SOURCE_DIR}/${_path}")
	cmake_path(NORMAL_PATH _source_file)
	cmake_path(IS_PREFIX CMAKE_SOURCE_DIR "${_source_file}" NORMALIZE _path_is_inside_source)
	if(NOT _path_is_inside_source)
		message(FATAL_ERROR "License path escapes the repository root: ${_path}")
	endif()
	if(NOT EXISTS "${_source_file}" OR IS_DIRECTORY "${_source_file}")
		message(FATAL_ERROR
			"Missing license/notice file for ${_component}: ${_source_file}"
		)
	endif()

	cmake_path(GET _path FILENAME _filename)
	set(_relative_destination
		"${_scalix_license_install_root}/third-party/${_component}/${_filename}"
	)
	if(_relative_destination IN_LIST _scalix_license_seen_destinations)
		message(FATAL_ERROR "Duplicate installed third-party license path: ${_relative_destination}")
	endif()
	list(APPEND _scalix_license_seen_destinations "${_relative_destination}")
	list(APPEND _scalix_license_expected_destinations "${_relative_destination}")

	string(MAKE_C_IDENTIFIER "${_component}" _component_id)
	set(_license_var "_scalix_license_${_component_id}")
	set(_source_var "_scalix_source_${_component_id}")
	set(_documents_var "_scalix_documents_${_component_id}")

	list(FIND _scalix_license_components "${_component}" _component_index)
	if(_component_index EQUAL -1)
		list(APPEND _scalix_license_components "${_component}")
		set(${_license_var} "${_license}")
		set(${_source_var} "${_source_url}")
		set(${_documents_var} "")
	else()
		if(NOT "${${_license_var}}" STREQUAL "${_license}")
			message(FATAL_ERROR
				"Component ${_component} has multiple SPDX identifiers in ${_scalix_license_manifest}"
			)
		endif()
		if(NOT "${${_source_var}}" STREQUAL "${_source_url}")
			message(FATAL_ERROR
				"Component ${_component} has multiple source URLs in ${_scalix_license_manifest}"
			)
		endif()
	endif()

	string(APPEND ${_documents_var}
		"  - [`${_filename}`](third-party/${_component}/${_filename})\n"
	)

	install(
		FILES "${_source_file}"
		DESTINATION "${_scalix_license_install_root}/third-party/${_component}"
	)
endforeach()

if(NOT _scalix_license_components)
	message(FATAL_ERROR "No third-party license entries found in ${_scalix_license_manifest}")
endif()

set(_scalix_generated_dir "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${_scalix_generated_dir}")
set(_scalix_notices_file "${_scalix_generated_dir}/THIRD_PARTY_NOTICES.md")
set(_scalix_expected_files "${_scalix_generated_dir}/THIRD_PARTY_LICENSES_INSTALL_LIST.txt")

file(WRITE "${_scalix_notices_file}"
	"# Third-Party Notices\n\n"
	"This file is generated from `THIRD_PARTY_LICENSES.csv`. Do not edit it manually.\n\n"
	"Scalix incorporates the third-party software listed below. Each component remains subject to its own license terms.\n\n"
	"**Scope:** This is a declared-dependency notice list, not an automatically generated inventory of every transitive dependency. Third-party code embedded within dependencies, system packages, and Bazel-fetched repositories may require additional review.\n\n"
	"License documents are included relative to this file under `third-party/<component>/`.\n"
)

foreach(_component IN LISTS _scalix_license_components)
	string(MAKE_C_IDENTIFIER "${_component}" _component_id)
	set(_license_var "_scalix_license_${_component_id}")
	set(_source_var "_scalix_source_${_component_id}")
	set(_documents_var "_scalix_documents_${_component_id}")

	file(APPEND "${_scalix_notices_file}"
		"\n## ${_component}\n\n"
		"- **License:** `${${_license_var}}`\n"
		"- **Source:** [Upstream repository](${${_source_var}})\n"
		"- **License and notice documents:**\n"
		"${${_documents_var}}"
	)
endforeach()

file(WRITE "${_scalix_expected_files}" "")
foreach(_destination IN LISTS _scalix_license_expected_destinations)
	file(APPEND "${_scalix_expected_files}" "${_destination}\n")
endforeach()

install(
	FILES
		"${CMAKE_SOURCE_DIR}/LICENSE"
		"${_scalix_notices_file}"
	DESTINATION "${_scalix_license_install_root}"
)
