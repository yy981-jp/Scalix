function(compile_shader NAME)
	set(SHADER_DIR
		${CMAKE_SOURCE_DIR}/shaders/${NAME}
	)

	set(VS_OUTPUT
		${CMAKE_BINARY_DIR}/runtime/vs_${NAME}.bin
	)

	set(FS_OUTPUT
		${CMAKE_BINARY_DIR}/runtime/fs_${NAME}.bin
	)

	add_custom_command(
		OUTPUT
			${VS_OUTPUT}
			${FS_OUTPUT}

		COMMAND ${CMAKE_COMMAND} -E make_directory
			${CMAKE_BINARY_DIR}/runtime

		COMMAND ${SHADERC_EXECUTABLE}
			-f ${SHADER_DIR}/vs.sc
			-o ${VS_OUTPUT}
			--type vertex
			--varyingdef ${SHADER_DIR}/varying.def.sc
			--profile s_5_0
			-i ${CMAKE_SOURCE_DIR}/external/bgfx/src

		COMMAND ${SHADERC_EXECUTABLE}
			-f ${SHADER_DIR}/fs.sc
			-o ${FS_OUTPUT}
			--type fragment
			--varyingdef ${SHADER_DIR}/varying.def.sc
			--profile s_5_0
			-i ${CMAKE_SOURCE_DIR}/external/bgfx/src

		DEPENDS
			${SHADER_DIR}/vs.sc
			${SHADER_DIR}/fs.sc
			${SHADER_DIR}/varying.def.sc

		VERBATIM
	)

endfunction()
