# wildcard_pattern : DLLファイル名にマッチさせるワイルドカードパターン (例: "libwebp-*.dll")
# search_dir        : DLLを探索するディレクトリ(ビルド出力先など)
# dest_dir          : インストール先(CPackのパッケージに入るパス)
#
# キーワード引数:
# PRE_EXCLUDE_REGEXES / POST_EXCLUDE_REGEXES
#   file(GET_RUNTIME_DEPENDENCIES)にそのまま渡す除外正規表現のリスト。
#   cpack.cmake側で計算したOSごとの_rd_pre_exclude/_rd_post_excludeを
#   ここに渡すことで、install(TARGETS ... RUNTIME_DEPENDENCIES ...)側の
#   除外設定と常に一致させる(二重管理を避ける)。
function(install_dlls_by_wildcard wildcard_pattern search_dir dest_dir)
    cmake_parse_arguments(ARG "" "" "PRE_EXCLUDE_REGEXES;POST_EXCLUDE_REGEXES" ${ARGN})

    # 渡された除外正規表現リストを、install(CODE "...")に埋め込むための
    # エスケープ済み・引用符付き文字列へ組み立てる(configure時に確定させる)
    set(_pre_str "")
    foreach(_p ${ARG_PRE_EXCLUDE_REGEXES})
        string(APPEND _pre_str " \\\"${_p}\\\"")
    endforeach()

    set(_post_str "")
    foreach(_p ${ARG_POST_EXCLUDE_REGEXES})
        string(APPEND _post_str " \\\"${_p}\\\"")
    endforeach()

    install(CODE "
		cmake_policy(SET CMP0207 NEW)

        set(_pattern \"${wildcard_pattern}\")
        set(_search_dir \"${search_dir}\")
        set(_dest_dir \"${dest_dir}\")

        # ワイルドカードで直接マッチするファイルを収集(正規表現不要)
        file(GLOB_RECURSE _matched_dlls_raw \"\${_search_dir}/\${_pattern}\")

        # file(GLOB_RECURSE)はディレクトリもマッチしてしまうため除外する
        # (例: 依存先サブモジュールが 'libwebp' という名前のディレクトリを
        #  持っている場合、それ自体が誤ってマッチしてしまう)
        set(_matched_dlls \"\")
        foreach(_f \${_matched_dlls_raw})
            if(NOT IS_DIRECTORY \"\${_f}\")
                list(APPEND _matched_dlls \"\${_f}\")
            endif()
        endforeach()

        if(_matched_dlls)
            # message(WARNING \"Matched DLLs: \${_matched_dlls}\")

            # 依存先を再帰的に解決
            file(GET_RUNTIME_DEPENDENCIES
                LIBRARIES \${_matched_dlls}
                RESOLVED_DEPENDENCIES_VAR _resolved
                UNRESOLVED_DEPENDENCIES_VAR _unresolved
                DIRECTORIES \${_search_dir} ${MSYS_BIN_DIR}
                PRE_EXCLUDE_REGEXES ${_pre_str_rd_pre_exclude}
                POST_EXCLUDE_REGEXES ${_rd_post_exclude}
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
