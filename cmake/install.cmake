set(_scalix_runtime_dependency_module "${CMAKE_CURRENT_LIST_DIR}/runtime_dependencies.cmake")
include("${_scalix_runtime_dependency_module}")
get_filename_component(SCALIX_COMPILER_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

install(
	TARGETS sl
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(
	DIRECTORY "${CMAKE_BINARY_DIR}/runtime/"
	DESTINATION "${CMAKE_INSTALL_BINDIR}/runtime"
)

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
		COMMAND \"${CMAKE_OBJDUMP}\"
		SEARCH_DIRECTORIES
			\"${EXTERNAL_INSTALL_PREFIX}/bin\"
			\"${EXTERNAL_INSTALL_PREFIX}/lib\"
			\"${EXTERNAL_INSTALL_PREFIX}/x64/mingw/bin\"
			\"${EXTERNAL_INSTALL_PREFIX}/x64/mingw/lib\"
			\"${SCALIX_COMPILER_BIN_DIR}\"
			\"${CMAKE_SOURCE_DIR}/sxmp/external/mediapipe/bazel-bin/external/sxmp\"
	)
")
