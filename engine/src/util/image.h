#pragma once

#include <def/fs.h>
#include <SDL3/SDL_surface.h>

// Loads an image from a UTF-8 filesystem path.
// The caller owns the returned surface and must call SDL_DestroySurface().
SDL_Surface* loadImageSurface(const fs::path& path);
