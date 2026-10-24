#pragma once
#include <scalix/ui/RmlUiRendererBgfx.h>
#include <def/vec2.h>
#include <def/def.h>
#include <def/view.h>
#include <util/path.h>


class RmlUiLayer {
	scalix::ui::RmlUiRendererBgfx backend;

public:
	RmlUiLayer(int height, int width) {
		scalix::ui::RmlUiRendererBgfxConfig config = {
			.view_id = (int)ViewId::game,
			.width = width,
			.height = height,
			.shader_directory = syspath.resource / "runtime",
			.texture_loader = nullptr
		};
		backend.init(config);
	}
};
