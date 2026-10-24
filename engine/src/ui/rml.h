#pragma once

#include <scalix/ui/RmlUiRendererBgfx.h>

#include <SDL3/SDL.h>

namespace Rml
{
	class Context;
}


class RmlUiLayer {
	scalix::ui::RmlUiRendererBgfx backend;
	Rml::Context* context = nullptr;
	bool initialized = false;

public:
	RmlUiLayer() = default;
	~RmlUiLayer();

	RmlUiLayer(const RmlUiLayer&) = delete;
	RmlUiLayer& operator=(const RmlUiLayer&) = delete;

	void init(int width, int height);
	void processEvent(const SDL_Event& event);
	void update();
	void render();
	void shutdown() noexcept;
};
