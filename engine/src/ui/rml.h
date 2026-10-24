#pragma once

#include <scalix/ui/RmlUiRendererBgfx.h>


class RmlUiLayer {
	scalix::ui::RmlUiRendererBgfx backend;

public:
	RmlUiLayer(int width, int height);
};
