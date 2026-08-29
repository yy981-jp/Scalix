#pragma once

#include <util/path.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <stdexcept>


class LogoRenderer {
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr;
	SDL_Window* window = nullptr;

public:
	LogoRenderer(SDL_Window* window)
		: window(window) {
		renderer = SDL_CreateRenderer(window, SDL_SOFTWARE_RENDERER);
		if (!renderer)
			throw std::runtime_error(SDL_GetError());

		SDL_Surface* surface =
			IMG_Load((syspath.resource / "assets" / "logo.webp").string().c_str());

		if (!surface) {
			SDL_DestroyRenderer(renderer);
			renderer = nullptr;
			throw std::runtime_error(SDL_GetError());
		}

		texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_DestroySurface(surface);

		if (!texture) {
			SDL_DestroyRenderer(renderer);
			renderer = nullptr;
			throw std::runtime_error(SDL_GetError());
		}
	}

	~LogoRenderer() {
		if (texture)
			SDL_DestroyTexture(texture);

		if (renderer)
			SDL_DestroyRenderer(renderer);
	}

	void draw() {
		SDL_RenderClear(renderer);
		int winW, winH;
		SDL_GetWindowSize(window, &winW, &winH);

		float texW, texH;
		SDL_GetTextureSize(texture, &texW, &texH);

		// スケール計算（小さい方に合わせる）
		float scaleX = (float)winW / texW;
		float scaleY = (float)winH / texH;
		float scale = scaleX < scaleY ? scaleX : scaleY;

		float drawW = (int)(texW * scale);
		float drawH = (int)(texH * scale);

		// 中央配置
		SDL_FRect dst = {
			(winW - drawW) / 2,
			(winH - drawH) / 2,
			drawW,
			drawH
		};

		SDL_RenderTexture(renderer, texture, NULL, &dst);
		SDL_RenderPresent(renderer);
	}
};