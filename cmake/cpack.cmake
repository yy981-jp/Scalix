get_filename_component(COMPILER_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)


if(WIN32)
	set(_rd_pre_exclude "api-ms-" "ext-ms-")
	set(_rd_post_exclude ".*system32/.*\\.dll")
elseif(APPLE)
	set(_rd_pre_exclude "")
	set(_rd_post_exclude "^/usr/lib/.*" "^/System/Library/.*")
elseif(UNIX)
	set(_rd_pre_exclude "^linux-vdso\\.so.*" "^ld-linux.*\\.so.*")
	set(_rd_post_exclude "^/lib/.*" "^/lib64/.*" "^/usr/lib/.*")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/install_dlls.cmake)

install(
	TARGETS ${EXE_NAME}
	RUNTIME_DEPENDENCIES
		DIRECTORIES
			# ${CMAKE_PREFIX_PATH}/bin
			${COMPILER_BIN_DIR}
			# "C:/msys64/mingw64/bin"
		PRE_EXCLUDE_REGEXES ${_rd_pre_exclude}
		POST_EXCLUDE_REGEXES ${_rd_post_exclude}
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION bin
		FRAMEWORK DESTINATION bin
	RUNTIME DESTINATION bin
)

# message(WARNING ${COMPILER_BIN_DIR})

# file(GET_RUNTIME_DEPENDENCIES
#     LIBRARIES "${CMAKE_PREFIX_PATH}/bin/libwebp-*"           # 依存関係を調べたい対象のDLLパス
#     DIRECTORIES "${CMAKE_PREFIX_PATH}/bin"      # DLLを探す検索ディレクトリのリスト
#     RESOLVED_DEPENDENCIES_VAR resolved_deps   # 解決された依存DLLのパスが入る変数
#     UNRESOLVED_DEPENDENCIES_VAR unresolved_deps # 未解決の依存DLLのパスが入る変数
# )


install_dlls_by_wildcard("libwebp*" "${COMPILER_BIN_DIR}" "bin")

# shaders
install(
	DIRECTORY ${CMAKE_BINARY_DIR}/runtime/
	DESTINATION bin/runtime
)

# resources
install(
	DIRECTORY ${CMAKE_SOURCE_DIR}/resources/
	DESTINATION resources
)

# tracker
install(
	DIRECTORY ${CMAKE_SOURCE_DIR}/tracker/build/camera.dist/
	DESTINATION bin/tracker
)


# CPack
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/dist")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${PROJECT_NAME})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

if(WIN32)
	# set(CPACK_GENERATOR "ZIP;NSIS")
	set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
	set(CPACK_GENERATOR "DragNDrop")
elseif(UNIX)
	set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
