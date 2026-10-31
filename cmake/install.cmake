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

if(APPLE)
	# The DMG contains one self-contained app bundle.
	install(
		TARGETS sl
		BUNDLE DESTINATION "."
	)

	set(_scalix_installed_executable "Scalix.app/Contents/MacOS/Scalix")
	set(_scalix_runtime_install_destination "Scalix.app/Contents/Frameworks")
	set(_scalix_resources_install_destination "Scalix.app/Contents/Resources")

	# Bundle the relocatable MediaPipe library before scanning the app's runtime
	# dependencies. The build action gives it an @rpath install name.
	install(
		FILES "${CMAKE_BINARY_DIR}/runtime-backend/libsxmp_be.dylib"
		DESTINATION "${_scalix_runtime_install_destination}"
	)
else()
	install(
		TARGETS sl
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	)

	set(_scalix_installed_executable "${CMAKE_INSTALL_BINDIR}/$<TARGET_FILE_NAME:sl>")
	set(_scalix_runtime_install_destination "${CMAKE_INSTALL_BINDIR}")
	set(_scalix_resources_install_destination "resources")
endif()

# install(
# 	DIRECTORY "${CMAKE_BINARY_DIR}/runtime/"
# 	DESTINATION "${CMAKE_INSTALL_BINDIR}/runtime"
# )

install(
	DIRECTORY "${CMAKE_SOURCE_DIR}/resources/"
	DESTINATION "${_scalix_resources_install_destination}"
	PATTERN ".git" EXCLUDE
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	# Install a freedesktop launcher and hicolor icons so desktop environments
	# can discover Scalix independently of SDL's per-window icon support.
	foreach(_size IN ITEMS 48 128 256)
		install(
			FILES "${CMAKE_SOURCE_DIR}/assets/dist/assets/icon-${_size}.png"
			DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_size}x${_size}/apps"
			RENAME "scalix.png"
		)
	endforeach()

	install(
		FILES "${CMAKE_SOURCE_DIR}/cmake/scalix.desktop"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
	)
endif()

install(CODE "
	include(\"${_scalix_runtime_dependency_module}\")
	scalix_resolve_runtime_dependencies(
		INSTALL_MODE
		STRICT
		EXECUTABLE \"\${CMAKE_INSTALL_PREFIX}/${_scalix_installed_executable}\"
		DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${_scalix_runtime_install_destination}\"
		PLATFORM \"${SCALIX_RUNTIME_DEPENDENCY_PLATFORM}\"
		TOOL \"${SCALIX_RUNTIME_DEPENDENCY_TOOL}\"
		${_scalix_runtime_dependency_command_code}
		SEARCH_DIRECTORIES ${_scalix_runtime_search_code}
	)
")

include("${CMAKE_SOURCE_DIR}/cmake/third_party_licenses.cmake")
