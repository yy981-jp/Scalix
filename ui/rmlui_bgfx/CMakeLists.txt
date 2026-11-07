find_package(bgfx CONFIG REQUIRED)
find_package(RmlUi CONFIG REQUIRED)

compile_shader_from_dir(rmlui_bgfx "${CMAKE_CURRENT_SOURCE_DIR}/shaders")

add_library(scalix_rmlui_bgfx STATIC
	src/RmlUiRendererBgfx.cpp
)

target_compile_features(scalix_rmlui_bgfx PUBLIC cxx_std_23)

target_include_directories(scalix_rmlui_bgfx
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(scalix_rmlui_bgfx
	PUBLIC
		bgfx::bgfx
		RmlUi::Core
)

add_custom_target(scalix_rmlui_bgfx_shaders
	DEPENDS ${rmlui_bgfx_SHADER_OUTPUTS}
)
add_dependencies(scalix_rmlui_bgfx scalix_rmlui_bgfx_shaders)

set_target_properties(scalix_rmlui_bgfx PROPERTIES
	OUTPUT_NAME scalix_rmlui_bgfx
	POSITION_INDEPENDENT_CODE ON
)
