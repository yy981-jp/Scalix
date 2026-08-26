get_filename_component(MSYS_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)


# wildcard_pattern : DLLファイル名にマッチさせるワイルドカードパターン (例: "libwebp-*.dll")
# search_dir        : DLLを探索するディレクトリ(ビルド出力先など)
# dest_dir          : インストール先(CPackのパッケージに入るパス)
function(install_dlls_by_wildcard wildcard_pattern search_dir dest_dir)
    install(CODE "
		cmake_policy(SET CMP0207 NEW)

        set(_pattern \"${wildcard_pattern}\")
        set(_search_dir \"${search_dir}\")
        set(_dest_dir \"${dest_dir}\")

        # ワイルドカードで直接マッチするDLLを収集(正規表現不要)
        file(GLOB_RECURSE _matched_dlls \"\${_search_dir}/\${_pattern}\")

        if(_matched_dlls)
            # message(WARNING \"Matched DLLs: \${_matched_dlls}\")

            # 依存先を再帰的に解決
            file(GET_RUNTIME_DEPENDENCIES
                LIBRARIES \${_matched_dlls}
                RESOLVED_DEPENDENCIES_VAR _resolved
                UNRESOLVED_DEPENDENCIES_VAR _unresolved
                DIRECTORIES \${_search_dir} \ ${MSYS_BIN_DIR}
                PRE_EXCLUDE_REGEXES \"api-ms-\" \"ext-ms-\"
                POST_EXCLUDE_REGEXES \".*[Ss]ystem32.*\"
            )

			# message(WARNING \"DEST DIR: \${_dest_dir}\")
			# message(WARNING \"MATCHED: \${_matched_dlls}\")
			# message(WARNING \"CMAKE_INSTALL_PREFIX: \${CMAKE_INSTALL_PREFIX}\")

			# マッチしたDLL本体をインストール
			file(INSTALL
				DESTINATION \"\${CMAKE_INSTALL_PREFIX}/\${_dest_dir}\"
				TYPE SHARED_LIBRARY
				FILES \${_matched_dlls}
			)
	            # 依存先DLLもインストール
            if(_resolved)
				file(INSTALL
					DESTINATION \"\${CMAKE_INSTALL_PREFIX}/\${_dest_dir}\"
					TYPE SHARED_LIBRARY
					FILES \${_resolved}
				)
            endif()

            if(_unresolved)
                message(WARNING \"解決できなかった依存: \${_unresolved}\")
            endif()
        else()
            message(WARNING \"パターン '\${_pattern}' にマッチするDLLが見つかりませんでした\")
        endif()
    " ALL_COMPONENTS)
endfunction()


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

install(
	TARGETS ${EXE_NAME}
	RUNTIME_DEPENDENCIES
		DIRECTORIES
			# ${CMAKE_PREFIX_PATH}/bin
			${MSYS_BIN_DIR}
			# "C:/msys64/mingw64/bin"
		PRE_EXCLUDE_REGEXES ${_rd_pre_exclude}
		POST_EXCLUDE_REGEXES ${_rd_post_exclude}
		RUNTIME DESTINATION bin
	RUNTIME DESTINATION bin
)

# message(WARNING ${MSYS_BIN_DIR})

# file(GET_RUNTIME_DEPENDENCIES
#     LIBRARIES "${CMAKE_PREFIX_PATH}/bin/libwebp-*"           # 依存関係を調べたい対象のDLLパス
#     DIRECTORIES "${CMAKE_PREFIX_PATH}/bin"      # DLLを探す検索ディレクトリのリスト
#     RESOLVED_DEPENDENCIES_VAR resolved_deps   # 解決された依存DLLのパスが入る変数
#     UNRESOLVED_DEPENDENCIES_VAR unresolved_deps # 未解決の依存DLLが入る変数
# )


install_dlls_by_wildcard("libwebp*" "${MSYS_BIN_DIR}" "bin")

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
