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
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(SHADER_PLATFORM linux)
else()
	message(FATAL_ERROR "Unsupported shader target platform: ${CMAKE_SYSTEM_NAME}")
endif()


# Compile one bgfx shader pair for a specific renderer profile.
function(compile_shader_variant NAME SHADER_DIR SHADER_OUTPUT_DIR SHADER_PROFILE OUTPUTS_VAR)
	set(VS_OUTPUT
		${SHADER_OUTPUT_DIR}/vs_${NAME}.bin
	)
	set(FS_OUTPUT
		${SHADER_OUTPUT_DIR}/fs_${NAME}.bin
	)

	add_custom_command(
		OUTPUT
			${VS_OUTPUT}
			${FS_OUTPUT}

		COMMAND ${CMAKE_COMMAND} -E make_directory
			${SHADER_OUTPUT_DIR}

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

	set(${OUTPUTS_VAR}
		${VS_OUTPUT}
		${FS_OUTPUT}
		PARENT_SCOPE
	)
endfunction()


# Compiles shaders into resources/runtime. Linux keeps separate variants because
# bgfx uses GLSL for OpenGL and SPIR-V for Vulkan.
function(compile_shader_from_dir NAME SHADER_DIR)
	set(_shader_outputs)

	if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		compile_shader_variant(
			"${NAME}"
			"${SHADER_DIR}"
			"${CMAKE_SOURCE_DIR}/resources/runtime/opengl"
			120
			_opengl_outputs
		)
		compile_shader_variant(
			"${NAME}"
			"${SHADER_DIR}"
			"${CMAKE_SOURCE_DIR}/resources/runtime/vulkan"
			spirv
			_vulkan_outputs
		)

		list(APPEND _shader_outputs ${_opengl_outputs} ${_vulkan_outputs})
	else()
		compile_shader_variant(
			"${NAME}"
			"${SHADER_DIR}"
			"${CMAKE_SOURCE_DIR}/resources/runtime"
			"${SHADER_PROFILE}"
			_default_outputs
		)
		list(APPEND _shader_outputs ${_default_outputs})
	endif()

	set(${NAME}_SHADER_OUTPUTS
		${_shader_outputs}
		PARENT_SCOPE
	)
endfunction()


# Preserve the original engine/shaders/<name> call site.
function(compile_shader NAME)
	compile_shader_from_dir(
		"${NAME}"
		"${CMAKE_SOURCE_DIR}/engine/shaders/${NAME}"
	)

	# Forward all renderer variants through the original call site.
	set(${NAME}_SHADER_OUTPUTS
		${${NAME}_SHADER_OUTPUTS}
		PARENT_SCOPE
	)
endfunction()
