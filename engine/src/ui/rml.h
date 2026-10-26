#pragma once

#include <RmlUi/Core/DataModelHandle.h>
#include <scalix/ui/RmlUiRendererBgfx.h>
#include <def/fs.h>

#include <SDL3/SDL.h>

namespace Rml {
	class Context;
	class ElementDocument;
}


class NoLogSystemInterface;

class RmlUiLayer {
	scalix::ui::RmlUiRendererBgfx backend;
	Rml::Context* context = nullptr;
	bool initialized = false;
	NoLogSystemInterface* noLogSystemInterface;
	Rml::DataModelConstructor model;
	Rml::DataModelHandle modelHandle;

public:
	RmlUiLayer() = default;
	~RmlUiLayer();

	RmlUiLayer(const RmlUiLayer&) = delete;
	RmlUiLayer& operator=(const RmlUiLayer&) = delete;

	void init(int width, int height);
	Rml::ElementDocument* loadDocument(const fs::path& path);
	void processEvent(const SDL_Event& event);
	void update();
	void render();
	void shutdown() noexcept;
	void dirty(const std::string& name);
	template <typename T>
	bool bind(const std::string& name, T* ptr) { return model.Bind(name, ptr); }
};
