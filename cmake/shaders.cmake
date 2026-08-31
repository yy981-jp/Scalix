find_program(SHADERC_EXECUTABLE shaderc
	HINTS
		${CMAKE_SOURCE_DIR}/external/install/release/bin
		# ${CMAKE_SOURCE_DIR}/external/install/debug/bin
	REQUIRED
)


if(WIN32)
	set(SHADER_PLATFORM windows)
	set(SHADER_PROFILE s_5_0)
elseif(APPLE)
	set(SHADER_PLATFORM osx)
	set(SHADER_PROFILE metal)
elseif(UNIX)
	set(SHADER_PLATFORM linux)
	set(SHADER_PROFILE spirv)
endif()


# Compiles engine/shaders/${NAME}/{vs,fs}.sc into runtime/{vs,fs}_${NAME}.bin
# under the build directory, and exposes the outputs to the caller as
# ${NAME}_SHADER_OUTPUTS so they can be wired as a build dependency.
function(compile_shader NAME)
	set(SHADER_DIR
		${CMAKE_SOURCE_DIR}/engine/shaders/${NAME}
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
			--platform ${SHADER_PLATFORM}
			--varyingdef ${SHADER_DIR}/varying.def.sc
			--profile ${SHADER_PROFILE}
			-i ${CMAKE_SOURCE_DIR}/external/bgfx/src

		COMMAND ${SHADERC_EXECUTABLE}
			-f ${SHADER_DIR}/fs.sc
			-o ${FS_OUTPUT}
			--type fragment
			--platform ${SHADER_PLATFORM}
			--varyingdef ${SHADER_DIR}/varying.def.sc
			--profile ${SHADER_PROFILE}
			-i ${CMAKE_SOURCE_DIR}/external/bgfx/src

		DEPENDS
			${SHADER_DIR}/vs.sc
			${SHADER_DIR}/fs.sc
			${SHADER_DIR}/varying.def.sc

		VERBATIM
	)

	set(${NAME}_SHADER_OUTPUTS ${VS_OUTPUT} ${FS_OUTPUT} PARENT_SCOPE)
endfunction()
