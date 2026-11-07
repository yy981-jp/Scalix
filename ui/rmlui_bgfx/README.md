# Scalix RmlUi bgfx Renderer

This target provides a first-pass RmlUi renderer that submits UI geometry to an
already-initialized bgfx instance.

## Ownership and frame order

- Scalix owns bgfx initialization, frame submission, and shutdown.
- The application reserves a bgfx view for the UI pass and configures its clear
  state so it does not clear the scene already rendered to the backbuffer.
- The UI view must execute after the scene view.
- Call `RmlUiRendererBgfx::Resize` when the framebuffer pixel size changes.
- Destroy RmlUi documents and call `Rml::Shutdown()` before destroying this
  renderer. Destroy the renderer before `bgfx::shutdown()`.

## Shaders

The project shader build produces `vs_rmlui_bgfx.bin` and
`fs_rmlui_bgfx.bin` in the build runtime shader directory. Pass that directory
through `RmlUiRendererBgfxConfig::shader_directory`.

## Image loading

Font atlases are uploaded through RmlUi's `GenerateTexture` callback. Loading
external images is injected through `RmlUiRendererBgfxConfig::texture_loader`
so this renderer does not add an SDL or image-decoder dependency.

The callback receives the resolved asset path and must return RGBA8 pixel data
with premultiplied alpha. Return `false` if the asset cannot be loaded.

## Current scope

The initial implementation covers compiled geometry, textures, alpha blending,
scissor clipping, and transforms. Advanced RmlUi rendering features such as clip
masks, layered filters, and custom decorator shaders are not implemented yet;
validate the specific RCSS features used by Scalix before relying on them.
