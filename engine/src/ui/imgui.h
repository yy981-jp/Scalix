#pragma once

#include <cstdint>

#include <SDL.h>
#include <bgfx/bgfx.h>
#include <dear-imgui/imgui.h>

// SDL input and bgfx rendering bridge for Dear ImGui.
// The application owns the frame order: beginFrame(), build ImGui widgets,
// then endFrame().  The bridge does not create a window or initialize bgfx.
class ImGuiBgfx final {
public:
	ImGuiBgfx() = default;
	~ImGuiBgfx();

	ImGuiBgfx(const ImGuiBgfx&) = delete;
	ImGuiBgfx& operator=(const ImGuiBgfx&) = delete;

	bool init(SDL_Window* window, bgfx::ViewId viewId = 255);
	void shutdown();

	// Submit every SDL event before beginFrame().
	void processEvent(const SDL_Event& event);
	void beginFrame(float deltaSeconds);
	void endFrame();

	[[nodiscard]] bool wantsMouseCapture() const;
	[[nodiscard]] bool wantsKeyboardCapture() const;
	[[nodiscard]] bool isInitialized() const { return initialized_; }

	// Use this for ImGui::Image()/ImageButton() texture IDs owned by bgfx.
	[[nodiscard]] static ImTextureID textureId(bgfx::TextureHandle texture);

private:
	SDL_Window* window_ = nullptr;
	bgfx::ViewId viewId_ = 255;
	bgfx::VertexLayout layout_;
	bgfx::ProgramHandle program_ = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle sampler_ = BGFX_INVALID_HANDLE;
	ImGuiContext* context_ = nullptr;
	bool initialized_ = false;

	void updateTextures(ImDrawData* drawData);
	void render(ImDrawData* drawData);
};
