// Derived from bgfx examples/common/imgui (BSD 2-Clause), adapted for Scalix.
// It intentionally uses only bgfx, bx, SDL2 and the existing Dear ImGui submodule.
#include <ui/imgui.h>

#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <cstring>

#include <bgfx/embedded_shader.h>
#include <bx/math.h>

#include <bgfx/examples/common/imgui/fs_ocornut_imgui.bin.h>
#include <bgfx/examples/common/imgui/vs_ocornut_imgui.bin.h>

namespace {

static const bgfx::EmbeddedShader kEmbeddedShaders[] = {
	BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
	BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
	BGFX_EMBEDDED_SHADER_END(),
};

ImGuiKey toImGuiKey(SDL_Keycode key) {
	switch (key) {
	case SDLK_TAB: return ImGuiKey_Tab;
	case SDLK_LEFT: return ImGuiKey_LeftArrow;
	case SDLK_RIGHT: return ImGuiKey_RightArrow;
	case SDLK_UP: return ImGuiKey_UpArrow;
	case SDLK_DOWN: return ImGuiKey_DownArrow;
	case SDLK_PAGEUP: return ImGuiKey_PageUp;
	case SDLK_PAGEDOWN: return ImGuiKey_PageDown;
	case SDLK_HOME: return ImGuiKey_Home;
	case SDLK_END: return ImGuiKey_End;
	case SDLK_INSERT: return ImGuiKey_Insert;
	case SDLK_DELETE: return ImGuiKey_Delete;
	case SDLK_BACKSPACE: return ImGuiKey_Backspace;
	case SDLK_SPACE: return ImGuiKey_Space;
	case SDLK_RETURN: return ImGuiKey_Enter;
	case SDLK_ESCAPE: return ImGuiKey_Escape;
	case SDLK_a: return ImGuiKey_A; case SDLK_b: return ImGuiKey_B;
	case SDLK_c: return ImGuiKey_C; case SDLK_d: return ImGuiKey_D;
	case SDLK_e: return ImGuiKey_E; case SDLK_f: return ImGuiKey_F;
	case SDLK_g: return ImGuiKey_G; case SDLK_h: return ImGuiKey_H;
	case SDLK_i: return ImGuiKey_I; case SDLK_j: return ImGuiKey_J;
	case SDLK_k: return ImGuiKey_K; case SDLK_l: return ImGuiKey_L;
	case SDLK_m: return ImGuiKey_M; case SDLK_n: return ImGuiKey_N;
	case SDLK_o: return ImGuiKey_O; case SDLK_p: return ImGuiKey_P;
	case SDLK_q: return ImGuiKey_Q; case SDLK_r: return ImGuiKey_R;
	case SDLK_s: return ImGuiKey_S; case SDLK_t: return ImGuiKey_T;
	case SDLK_u: return ImGuiKey_U; case SDLK_v: return ImGuiKey_V;
	case SDLK_w: return ImGuiKey_W; case SDLK_x: return ImGuiKey_X;
	case SDLK_y: return ImGuiKey_Y; case SDLK_z: return ImGuiKey_Z;
	case SDLK_0: return ImGuiKey_0; case SDLK_1: return ImGuiKey_1;
	case SDLK_2: return ImGuiKey_2; case SDLK_3: return ImGuiKey_3;
	case SDLK_4: return ImGuiKey_4; case SDLK_5: return ImGuiKey_5;
	case SDLK_6: return ImGuiKey_6; case SDLK_7: return ImGuiKey_7;
	case SDLK_8: return ImGuiKey_8; case SDLK_9: return ImGuiKey_9;
	case SDLK_F1: return ImGuiKey_F1; case SDLK_F2: return ImGuiKey_F2;
	case SDLK_F3: return ImGuiKey_F3; case SDLK_F4: return ImGuiKey_F4;
	case SDLK_F5: return ImGuiKey_F5; case SDLK_F6: return ImGuiKey_F6;
	case SDLK_F7: return ImGuiKey_F7; case SDLK_F8: return ImGuiKey_F8;
	case SDLK_F9: return ImGuiKey_F9; case SDLK_F10: return ImGuiKey_F10;
	case SDLK_F11: return ImGuiKey_F11; case SDLK_F12: return ImGuiKey_F12;
	default: return ImGuiKey_None;
	}
}

bgfx::TextureHandle textureFromId(ImTextureID id) {
	const uintptr_t value = reinterpret_cast<uintptr_t>(id);
	bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
	if (value != 0) handle.idx = static_cast<uint16_t>(value - 1);
	return handle;
}

} // namespace


ImGuiBgfx::~ImGuiBgfx() { shutdown(); }

bool ImGuiBgfx::init(SDL_Window* window, bgfx::ViewId viewId) {
	if (initialized_ || window == nullptr) return false;

	IMGUI_CHECKVERSION();
	context_ = ImGui::CreateContext();
	ImGui::SetCurrentContext(context_);
	ImGuiIO& io = ImGui::GetIO();
	io.BackendPlatformName = "scalix_sdl2";
	io.BackendRendererName = "scalix_bgfx";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.IniFilename = nullptr;
	ImGui::StyleColorsDark();

	window_ = window;
	viewId_ = viewId;
	layout_.begin()
		.add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
		.end();

	const bgfx::RendererType::Enum renderer = bgfx::getRendererType();
	program_ = bgfx::createProgram(
		bgfx::createEmbeddedShader(kEmbeddedShaders, renderer, "vs_ocornut_imgui"),
		bgfx::createEmbeddedShader(kEmbeddedShaders, renderer, "fs_ocornut_imgui"), true);
	sampler_ = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);
	initialized_ = bgfx::isValid(program_) && bgfx::isValid(sampler_);
	if (!initialized_) shutdown();
	return initialized_;
}

void ImGuiBgfx::shutdown() {
	if (!initialized_ && context_ == nullptr) return;
	if (context_ != nullptr) {
		ImGui::SetCurrentContext(context_);
		for (ImTextureData* texture : ImGui::GetPlatformIO().Textures) {
			if (texture->BackendUserData != nullptr) {
				bgfx::destroy(*static_cast<bgfx::TextureHandle*>(texture->BackendUserData));
				delete static_cast<bgfx::TextureHandle*>(texture->BackendUserData);
			}
		}
		ImGui::DestroyContext(context_);
	}
	if (bgfx::isValid(sampler_)) bgfx::destroy(sampler_);
	if (bgfx::isValid(program_)) bgfx::destroy(program_);
	window_ = nullptr; context_ = nullptr; initialized_ = false;
	sampler_ = BGFX_INVALID_HANDLE; program_ = BGFX_INVALID_HANDLE;
}

void ImGuiBgfx::processEvent(const SDL_Event& event) {
	if (!initialized_) return;
	ImGui::SetCurrentContext(context_);
	ImGuiIO& io = ImGui::GetIO();
	switch (event.type) {
	case SDL_MOUSEMOTION: io.AddMousePosEvent(float(event.motion.x), float(event.motion.y)); break;
	case SDL_MOUSEWHEEL: io.AddMouseWheelEvent(float(event.wheel.x), float(event.wheel.y)); break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP: {
		const int button = event.button.button == SDL_BUTTON_LEFT ? 0 : event.button.button == SDL_BUTTON_RIGHT ? 1 : event.button.button == SDL_BUTTON_MIDDLE ? 2 : -1;
		if (button >= 0) io.AddMouseButtonEvent(button, event.type == SDL_MOUSEBUTTONDOWN);
		break;
	}
	case SDL_TEXTINPUT: io.AddInputCharactersUTF8(event.text.text); break;
	case SDL_KEYDOWN:
	case SDL_KEYUP: {
		const bool down = event.type == SDL_KEYDOWN;
		const ImGuiKey key = toImGuiKey(event.key.keysym.sym);
		if (key != ImGuiKey_None) io.AddKeyEvent(key, down);
		const SDL_Keymod modifiers = SDL_GetModState();
		io.AddKeyEvent(ImGuiMod_Ctrl, (modifiers & KMOD_CTRL) != 0);
		io.AddKeyEvent(ImGuiMod_Shift, (modifiers & KMOD_SHIFT) != 0);
		io.AddKeyEvent(ImGuiMod_Alt, (modifiers & KMOD_ALT) != 0);
		io.AddKeyEvent(ImGuiMod_Super, (modifiers & KMOD_GUI) != 0);
		break;
	}
	default: break;
	}
}

void ImGuiBgfx::beginFrame(float deltaSeconds) {
	if (!initialized_) return;
	ImGui::SetCurrentContext(context_);
	int width = 0, height = 0;
	SDL_GetWindowSize(window_, &width, &height);
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(float(width), float(height));
	io.DeltaTime = std::max(deltaSeconds, 1.0f / 1000.0f);
	ImGui::NewFrame();
}

void ImGuiBgfx::updateTextures(ImDrawData* drawData) {
	if (drawData->Textures == nullptr) return;
	for (ImTextureData* texture : *drawData->Textures) {
		if (texture->Status == ImTextureStatus_WantCreate) {
			auto* handle = new bgfx::TextureHandle(bgfx::createTexture2D(uint16_t(texture->Width), uint16_t(texture->Height), false, 1, bgfx::TextureFormat::RGBA8));
			texture->BackendUserData = handle;
			texture->SetTexID(textureId(*handle));
			texture->SetStatus(ImTextureStatus_OK);
		}
		if (texture->Status == ImTextureStatus_WantUpdates) {
			auto* handle = static_cast<bgfx::TextureHandle*>(texture->BackendUserData);
			for (const ImTextureRect& rect : texture->Updates) {
				const uint8_t* src = static_cast<const uint8_t*>(texture->GetPixelsAt(rect.x, rect.y));
				const uint32_t rowBytes = rect.w * texture->BytesPerPixel;
				const bgfx::Memory* memory = bgfx::alloc(rect.h * rowBytes);
				for (int row = 0; row < rect.h; ++row) std::memcpy(memory->data + row * rowBytes, src + row * texture->GetPitch(), rowBytes);
				bgfx::updateTexture2D(*handle, 0, 0, uint16_t(rect.x), uint16_t(rect.y), uint16_t(rect.w), uint16_t(rect.h), memory);
			}
			texture->SetStatus(ImTextureStatus_OK);
		}
		if (texture->Status == ImTextureStatus_WantDestroy) {
			auto* handle = static_cast<bgfx::TextureHandle*>(texture->BackendUserData);
			if (handle != nullptr) { bgfx::destroy(*handle); delete handle; }
			texture->BackendUserData = nullptr;
			texture->SetTexID(ImTextureID_Invalid);
			texture->SetStatus(ImTextureStatus_Destroyed);
		}
	}
}

void ImGuiBgfx::render(ImDrawData* drawData) {
	updateTextures(drawData);
	const int displayWidth = int(drawData->DisplaySize.x * drawData->FramebufferScale.x);
	const int displayHeight = int(drawData->DisplaySize.y * drawData->FramebufferScale.y);
	if (displayWidth <= 0 || displayHeight <= 0) return;

	bgfx::setViewName(viewId_, "ImGui");
	bgfx::setViewMode(viewId_, bgfx::ViewMode::Sequential);
	float ortho[16];
	const ImVec2 displayPos = drawData->DisplayPos;
	bx::mtxOrtho(ortho, displayPos.x, displayPos.x + drawData->DisplaySize.x, displayPos.y + drawData->DisplaySize.y, displayPos.y, 0.0f, 1000.0f, 0.0f, bgfx::getCaps()->homogeneousDepth);
	bgfx::setViewTransform(viewId_, nullptr, ortho);
	bgfx::setViewRect(viewId_, 0, 0, uint16_t(displayWidth), uint16_t(displayHeight));

	for (int listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex) {
		const ImDrawList* list = drawData->CmdLists[listIndex];
		const uint32_t vertices = uint32_t(list->VtxBuffer.Size);
		const uint32_t indices = uint32_t(list->IdxBuffer.Size);
		if (vertices != bgfx::getAvailTransientVertexBuffer(vertices, layout_) || indices != bgfx::getAvailTransientIndexBuffer(indices)) break;
		bgfx::TransientVertexBuffer tvb; bgfx::TransientIndexBuffer tib;
		bgfx::allocTransientVertexBuffer(&tvb, vertices, layout_);
		bgfx::allocTransientIndexBuffer(&tib, indices, sizeof(ImDrawIdx) == 4);
		std::memcpy(tvb.data, list->VtxBuffer.Data, vertices * sizeof(ImDrawVert));
		std::memcpy(tib.data, list->IdxBuffer.Data, indices * sizeof(ImDrawIdx));

		for (const ImDrawCmd& command : list->CmdBuffer) {
			if (command.UserCallback != nullptr) {
				if (command.UserCallback != ImGui::GetPlatformIO().DrawCallback_ResetRenderState) command.UserCallback(list, &command);
				continue;
			}
			const ImVec4 clip = command.ClipRect;
			const int x = std::max(0, int((clip.x - displayPos.x) * drawData->FramebufferScale.x));
			const int y = std::max(0, int((clip.y - displayPos.y) * drawData->FramebufferScale.y));
			const int z = std::min(displayWidth, int((clip.z - displayPos.x) * drawData->FramebufferScale.x));
			const int w = std::min(displayHeight, int((clip.w - displayPos.y) * drawData->FramebufferScale.y));
			if (x >= z || y >= w || command.ElemCount == 0) continue;
			bgfx::setScissor(uint16_t(x), uint16_t(y), uint16_t(z - x), uint16_t(w - y));
			bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA));
			bgfx::setTexture(0, sampler_, textureFromId(command.GetTexID()));
			bgfx::setVertexBuffer(0, &tvb, command.VtxOffset, vertices - command.VtxOffset);
			bgfx::setIndexBuffer(&tib, command.IdxOffset, command.ElemCount);
			bgfx::submit(viewId_, program_);
		}
	}
}

void ImGuiBgfx::endFrame() {
	if (!initialized_) return;
	ImGui::SetCurrentContext(context_);
	ImGui::Render();
	render(ImGui::GetDrawData());
}

bool ImGuiBgfx::wantsMouseCapture() const { return initialized_ && ImGui::GetIO().WantCaptureMouse; }
bool ImGuiBgfx::wantsKeyboardCapture() const { return initialized_ && ImGui::GetIO().WantCaptureKeyboard; }

ImTextureID ImGuiBgfx::textureId(bgfx::TextureHandle texture) {
	return bgfx::isValid(texture) ? reinterpret_cast<ImTextureID>(uintptr_t(texture.idx) + 1) : ImTextureID_Invalid;
}
