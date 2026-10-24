#pragma once

#include <RmlUi/Core/RenderInterface.h>
#include <bgfx/bgfx.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <unordered_map>
#include <vector>

namespace scalix::ui
{
	// The loader must return RGBA8 pixels with premultiplied alpha, matching
	// RmlUi::RenderInterface::GenerateTexture's pixel format contract.
	using RmlUiTextureLoader = std::function<bool(
		const Rml::String& source,
		Rml::Vector2i& dimensions,
		std::vector<Rml::byte>& rgba_pixels)>;

	struct RmlUiRendererBgfxConfig
	{
		bgfx::ViewId view_id = 0;
		int width = 1;
		int height = 1;
		std::filesystem::path shader_directory;
		RmlUiTextureLoader texture_loader;
	};

	// A minimal RmlUi renderer that submits 2D UI geometry to an existing bgfx
	// instance. The application owns bgfx initialization, view ordering, frame
	// submission, and shutdown.
	//
	// Lifetime: destroy this renderer after Rml::Shutdown(), but before
	// bgfx::shutdown(). Its view must be configured by the application so that
	// the UI pass executes after the scene and does not clear the backbuffer.
	class RmlUiRendererBgfx final : public Rml::RenderInterface
	{
	public:
		void init(const RmlUiRendererBgfxConfig& config);
		~RmlUiRendererBgfx() override;

		RmlUiRendererBgfx& operator=(const RmlUiRendererBgfx&) = delete;

		void Resize(int width, int height);

		Rml::CompiledGeometryHandle CompileGeometry(
			Rml::Span<const Rml::Vertex> vertices,
			Rml::Span<const int> indices) override;
		void RenderGeometry(
			Rml::CompiledGeometryHandle geometry,
			Rml::Vector2f translation,
			Rml::TextureHandle texture) override;
		void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

		Rml::TextureHandle LoadTexture(
			Rml::Vector2i& texture_dimensions,
			const Rml::String& source) override;
		Rml::TextureHandle GenerateTexture(
			Rml::Span<const Rml::byte> source,
			Rml::Vector2i source_dimensions) override;
		void ReleaseTexture(Rml::TextureHandle texture) override;

		void EnableScissorRegion(bool enable) override;
		void SetScissorRegion(Rml::Rectanglei region) override;
		void SetTransform(const Rml::Matrix4f* transform) override;

	private:
		struct Geometry
		{
			bgfx::VertexBufferHandle vertex_buffer = BGFX_INVALID_HANDLE;
			bgfx::IndexBufferHandle index_buffer = BGFX_INVALID_HANDLE;
		};

		void UpdateProjection();
		void DestroyResources() noexcept;
		bgfx::TextureHandle CreateTexture(
			const Rml::byte* pixels,
			size_t pixel_bytes,
			Rml::Vector2i dimensions);

		RmlUiRendererBgfxConfig m_config;
		bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
		bgfx::UniformHandle m_projection_uniform = BGFX_INVALID_HANDLE;
		bgfx::UniformHandle m_transform_uniform = BGFX_INVALID_HANDLE;
		bgfx::UniformHandle m_translation_uniform = BGFX_INVALID_HANDLE;
		bgfx::UniformHandle m_sampler_uniform = BGFX_INVALID_HANDLE;
		bgfx::TextureHandle m_white_texture = BGFX_INVALID_HANDLE;
		bgfx::VertexLayout m_vertex_layout{};

		Rml::Matrix4f m_projection = Rml::Matrix4f::Identity();
		Rml::Matrix4f m_transform = Rml::Matrix4f::Identity();

		std::unordered_map<Rml::CompiledGeometryHandle, Geometry> m_geometry;
		std::unordered_map<Rml::TextureHandle, bgfx::TextureHandle> m_textures;
		Rml::CompiledGeometryHandle m_next_geometry = 1;
		Rml::TextureHandle m_next_texture = 1;

		int m_width = 1;
		int m_height = 1;
		bool m_scissor_enabled = false;
		Rml::Rectanglei m_scissor_region =
			Rml::Rectanglei::FromPositionSize({0, 0}, {1, 1});
	};
}
