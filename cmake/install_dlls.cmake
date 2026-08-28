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
