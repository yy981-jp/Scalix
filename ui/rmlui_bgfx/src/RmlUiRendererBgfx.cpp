#include <scalix/ui/RmlUiRendererBgfx.h>

#include <RmlUi/Core/Log.h>
#include <bx/math.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
	struct BgfxVertex
	{
		float x;
		float y;
		Rml::byte r;
		Rml::byte g;
		Rml::byte b;
		Rml::byte a;
		float u;
		float v;
	};

	std::vector<Rml::byte> ReadBinaryFile(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file)
		{
			throw std::runtime_error("Failed to open bgfx shader: " + path.string());
		}

		return {
			std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>()
		};
	}

	bgfx::ShaderHandle LoadShader(const std::filesystem::path& path)
	{
		auto bytes = ReadBinaryFile(path);
		if (bytes.empty() || bytes.size() > std::numeric_limits<uint32_t>::max())
		{
			throw std::runtime_error("Invalid bgfx shader binary: " + path.string());
		}

		const auto* memory = bgfx::copy(bytes.data(), static_cast<uint32_t>(bytes.size()));
		const auto shader = bgfx::createShader(memory);
		if (!bgfx::isValid(shader))
		{
			throw std::runtime_error("bgfx rejected shader binary: " + path.string());
		}

		return shader;
	}
}

namespace scalix::ui
{
	void RmlUiRendererBgfx::init(const RmlUiRendererBgfxConfig& config) {
		if (bgfx::isValid(m_program) || bgfx::isValid(m_white_texture))
			throw std::logic_error("RmlUiRendererBgfx is already initialized.");

		m_config = config;

		if (bgfx::getCaps() == nullptr)
		{
			throw std::runtime_error("RmlUiRendererBgfx requires an initialized bgfx instance.");
		}
		if (m_config.width <= 0 || m_config.height <= 0)
		{
			throw std::invalid_argument("RmlUiRendererBgfx viewport dimensions must be positive.");
		}

		try
		{
			m_vertex_layout
				.begin()
				.add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
				.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
				.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
				.end();

			const auto vertex_shader = LoadShader(m_config.shader_directory / "vs_rmlui_bgfx.bin");
			bgfx::ShaderHandle fragment_shader = BGFX_INVALID_HANDLE;
			try
			{
				fragment_shader = LoadShader(m_config.shader_directory / "fs_rmlui_bgfx.bin");
			}
			catch (...)
			{
				bgfx::destroy(vertex_shader);
				throw;
			}

			m_program = bgfx::createProgram(vertex_shader, fragment_shader, true);
			if (!bgfx::isValid(m_program))
			{
				throw std::runtime_error("Failed to create the RmlUi bgfx shader program.");
			}

			m_projection_uniform = bgfx::createUniform("u_projection", bgfx::UniformType::Mat4);
			m_transform_uniform = bgfx::createUniform("u_transform", bgfx::UniformType::Mat4);
			m_translation_uniform = bgfx::createUniform("u_translate", bgfx::UniformType::Vec4);
			m_sampler_uniform = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);

			const std::array<Rml::byte, 4> white_pixel{255, 255, 255, 255};
			m_white_texture = bgfx::createTexture2D(
				1, 1, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE,
				bgfx::copy(white_pixel.data(), static_cast<uint32_t>(white_pixel.size())));

			if (!bgfx::isValid(m_projection_uniform) ||
				!bgfx::isValid(m_transform_uniform) ||
				!bgfx::isValid(m_translation_uniform) ||
				!bgfx::isValid(m_sampler_uniform) ||
				!bgfx::isValid(m_white_texture))
			{
				throw std::runtime_error("Failed to create RmlUi bgfx resources.");
			}

			Resize(m_config.width, m_config.height);
		}
		catch (...)
		{
			DestroyResources();
			throw;
		}
	}

	void RmlUiRendererBgfx::shutdown() noexcept
	{
		DestroyResources();
	}

	RmlUiRendererBgfx::~RmlUiRendererBgfx()
	{
		shutdown();
	}

	void RmlUiRendererBgfx::DestroyResources() noexcept
	{
		for (const auto& [handle, geometry] : m_geometry)
		{
			(void)handle;
			if (bgfx::isValid(geometry.vertex_buffer))
				bgfx::destroy(geometry.vertex_buffer);
			if (bgfx::isValid(geometry.index_buffer))
				bgfx::destroy(geometry.index_buffer);
		}
		m_geometry.clear();

		for (const auto& [handle, texture] : m_textures)
		{
			(void)handle;
			if (bgfx::isValid(texture))
				bgfx::destroy(texture);
		}
		m_textures.clear();

		if (bgfx::isValid(m_white_texture))
			bgfx::destroy(m_white_texture);
		if (bgfx::isValid(m_sampler_uniform))
			bgfx::destroy(m_sampler_uniform);
		if (bgfx::isValid(m_translation_uniform))
			bgfx::destroy(m_translation_uniform);
		if (bgfx::isValid(m_transform_uniform))
			bgfx::destroy(m_transform_uniform);
		if (bgfx::isValid(m_projection_uniform))
			bgfx::destroy(m_projection_uniform);
		if (bgfx::isValid(m_program))
			bgfx::destroy(m_program);

		m_white_texture = BGFX_INVALID_HANDLE;
		m_sampler_uniform = BGFX_INVALID_HANDLE;
		m_translation_uniform = BGFX_INVALID_HANDLE;
		m_transform_uniform = BGFX_INVALID_HANDLE;
		m_projection_uniform = BGFX_INVALID_HANDLE;
		m_program = BGFX_INVALID_HANDLE;
	}

	void RmlUiRendererBgfx::Resize(int width, int height)
	{
		if (width <= 0 || height <= 0 ||
			width > std::numeric_limits<uint16_t>::max() ||
			height > std::numeric_limits<uint16_t>::max())
		{
			throw std::invalid_argument("RmlUiRendererBgfx received invalid viewport dimensions.");
		}

		m_width = width;
		m_height = height;
		m_scissor_region = Rml::Rectanglei::FromPositionSize({0, 0}, {width, height});
		bgfx::setViewRect(m_config.view_id, 0, 0, static_cast<uint16_t>(width), static_cast<uint16_t>(height));
		UpdateProjection();
	}

	void RmlUiRendererBgfx::UpdateProjection()
	{
		bx::mtxOrtho(
			m_projection.data(),
			0.0f, static_cast<float>(m_width),
			static_cast<float>(m_height), 0.0f,
			-10000.0f, 10000.0f, 0.0f,
			bgfx::getCaps()->homogeneousDepth);
	}

	Rml::CompiledGeometryHandle RmlUiRendererBgfx::CompileGeometry(
		Rml::Span<const Rml::Vertex> vertices,
		Rml::Span<const int> indices)
	{
		if (vertices.empty() || indices.empty())
			return 0;

		std::vector<BgfxVertex> bgfx_vertices;
		bgfx_vertices.reserve(vertices.size());
		for (const Rml::Vertex& vertex : vertices)
		{
			bgfx_vertices.push_back({
				vertex.position.x,
				vertex.position.y,
				vertex.colour.red,
				vertex.colour.green,
				vertex.colour.blue,
				vertex.colour.alpha,
				vertex.tex_coord.x,
				vertex.tex_coord.y
			});
		}

		std::vector<uint32_t> bgfx_indices;
		bgfx_indices.reserve(indices.size());
		for (const int index : indices)
		{
			if (index < 0 || static_cast<size_t>(index) >= vertices.size())
				return 0;
			bgfx_indices.push_back(static_cast<uint32_t>(index));
		}

		const auto vertex_bytes = bgfx_vertices.size() * sizeof(BgfxVertex);
		const auto index_bytes = bgfx_indices.size() * sizeof(uint32_t);
		if (vertex_bytes > std::numeric_limits<uint32_t>::max() ||
			index_bytes > std::numeric_limits<uint32_t>::max())
		{
			return 0;
		}

		Geometry geometry;
		geometry.vertex_buffer = bgfx::createVertexBuffer(
			bgfx::copy(bgfx_vertices.data(), static_cast<uint32_t>(vertex_bytes)),
			m_vertex_layout);
		geometry.index_buffer = bgfx::createIndexBuffer(
			bgfx::copy(bgfx_indices.data(), static_cast<uint32_t>(index_bytes)),
			BGFX_BUFFER_INDEX32);

		if (!bgfx::isValid(geometry.vertex_buffer) || !bgfx::isValid(geometry.index_buffer))
		{
			if (bgfx::isValid(geometry.vertex_buffer))
				bgfx::destroy(geometry.vertex_buffer);
			if (bgfx::isValid(geometry.index_buffer))
				bgfx::destroy(geometry.index_buffer);
			return 0;
		}

		const auto handle = m_next_geometry++;
		m_geometry.emplace(handle, geometry);
		return handle;
	}

	void RmlUiRendererBgfx::RenderGeometry(
		Rml::CompiledGeometryHandle geometry_handle,
		Rml::Vector2f translation,
		Rml::TextureHandle texture_handle)
	{
		const auto geometry_iter = m_geometry.find(geometry_handle);
		if (geometry_iter == m_geometry.end())
			return;

		if (m_scissor_enabled)
		{
			if (m_scissor_region.Width() <= 0 || m_scissor_region.Height() <= 0)
				return;
			bgfx::setScissor(
				static_cast<uint16_t>(m_scissor_region.Left()),
				static_cast<uint16_t>(m_scissor_region.Top()),
				static_cast<uint16_t>(m_scissor_region.Width()),
				static_cast<uint16_t>(m_scissor_region.Height()));
		}
		else
		{
			bgfx::setScissor();
		}

		bgfx::TextureHandle texture = m_white_texture;
		const auto texture_iter = m_textures.find(texture_handle);
		if (texture_iter != m_textures.end())
			texture = texture_iter->second;

		const float translation_data[4] = {translation.x, translation.y, 0.0f, 0.0f};
		bgfx::setUniform(m_projection_uniform, m_projection.data());
		bgfx::setUniform(m_transform_uniform, m_transform.data());
		bgfx::setUniform(m_translation_uniform, translation_data);
		bgfx::setTexture(0, m_sampler_uniform, texture);
		bgfx::setVertexBuffer(0, geometry_iter->second.vertex_buffer);
		bgfx::setIndexBuffer(geometry_iter->second.index_buffer);
		bgfx::setState(
			BGFX_STATE_WRITE_RGB |
			BGFX_STATE_WRITE_A |
			BGFX_STATE_MSAA |
			BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA));
		bgfx::submit(m_config.view_id, m_program);
	}

	void RmlUiRendererBgfx::ReleaseGeometry(Rml::CompiledGeometryHandle geometry_handle)
	{
		const auto iter = m_geometry.find(geometry_handle);
		if (iter == m_geometry.end())
			return;

		if (bgfx::isValid(iter->second.vertex_buffer))
			bgfx::destroy(iter->second.vertex_buffer);
		if (bgfx::isValid(iter->second.index_buffer))
			bgfx::destroy(iter->second.index_buffer);
		m_geometry.erase(iter);
	}

	bgfx::TextureHandle RmlUiRendererBgfx::CreateTexture(
		const Rml::byte* pixels,
		size_t pixel_bytes,
		Rml::Vector2i dimensions)
	{
		if (pixels == nullptr || dimensions.x <= 0 || dimensions.y <= 0 ||
			dimensions.x > std::numeric_limits<uint16_t>::max() ||
			dimensions.y > std::numeric_limits<uint16_t>::max())
		{
			return BGFX_INVALID_HANDLE;
		}

		const size_t width = static_cast<size_t>(dimensions.x);
		const size_t height = static_cast<size_t>(dimensions.y);
		if (width > std::numeric_limits<size_t>::max() / height / 4 ||
			width * height * 4 != pixel_bytes ||
			pixel_bytes > std::numeric_limits<uint32_t>::max())
		{
			return BGFX_INVALID_HANDLE;
		}

		if (!bgfx::isTextureValid(0, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE))
			return BGFX_INVALID_HANDLE;

		return bgfx::createTexture2D(
			static_cast<uint16_t>(dimensions.x),
			static_cast<uint16_t>(dimensions.y),
			false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE,
			bgfx::copy(pixels, static_cast<uint32_t>(pixel_bytes)));
	}

	Rml::TextureHandle RmlUiRendererBgfx::LoadTexture(
		Rml::Vector2i& texture_dimensions,
		const Rml::String& source)
	{
		if (!m_config.texture_loader)
			return 0;

		std::vector<Rml::byte> pixels;
		if (!m_config.texture_loader(source, texture_dimensions, pixels))
			return 0;

		const auto texture = CreateTexture(pixels.data(), pixels.size(), texture_dimensions);
		if (!bgfx::isValid(texture))
			return 0;

		const auto handle = m_next_texture++;
		m_textures.emplace(handle, texture);
		return handle;
	}

	Rml::TextureHandle RmlUiRendererBgfx::GenerateTexture(
		Rml::Span<const Rml::byte> source,
		Rml::Vector2i source_dimensions)
	{
		const auto texture = CreateTexture(source.data(), source.size(), source_dimensions);
		if (!bgfx::isValid(texture))
			return 0;

		const auto handle = m_next_texture++;
		m_textures.emplace(handle, texture);
		return handle;
	}

	void RmlUiRendererBgfx::ReleaseTexture(Rml::TextureHandle texture_handle)
	{
		const auto iter = m_textures.find(texture_handle);
		if (iter == m_textures.end())
			return;

		if (bgfx::isValid(iter->second))
			bgfx::destroy(iter->second);
		m_textures.erase(iter);
	}

	void RmlUiRendererBgfx::EnableScissorRegion(bool enable)
	{
		m_scissor_enabled = enable;
	}

	void RmlUiRendererBgfx::SetScissorRegion(Rml::Rectanglei region)
	{
		if (!region.Valid())
		{
			m_scissor_region = Rml::Rectanglei::FromPositionSize({0, 0}, {0, 0});
			return;
		}

		const int left = std::clamp(region.Left(), 0, m_width);
		const int top = std::clamp(region.Top(), 0, m_height);
		const int right = std::clamp(region.Right(), left, m_width);
		const int bottom = std::clamp(region.Bottom(), top, m_height);
		m_scissor_region = Rml::Rectanglei::FromPositionSize(
			{left, top}, {right - left, bottom - top});
	}

	void RmlUiRendererBgfx::SetTransform(const Rml::Matrix4f* transform)
	{
		m_transform = transform ? *transform : Rml::Matrix4f::Identity();
	}
}
