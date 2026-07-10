#pragma once

#include <def/def.h>

#include <SDL.h>
#include <SDL_image.h>


class LogoRenderer {
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_Window* window;

public:
    LogoRenderer(SDL_Window* window): window(window) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        SDL_Surface* surface = IMG_Load((Assets + "logo.webp").c_str());
        if (!surface) throw std::runtime_error("logo not found");
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    ~LogoRenderer() {
    	SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
    }

    void draw() {
        SDL_RenderClear(renderer);
        int winW, winH;
        SDL_GetWindowSize(window, &winW, &winH);

        int texW, texH;
        SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);

        // スケール計算（小さい方に合わせる）
        float scaleX = (float)winW / texW;
        float scaleY = (float)winH / texH;
        float scale = scaleX < scaleY ? scaleX : scaleY;

        int drawW = (int)(texW * scale);
        int drawH = (int)(texH * scale);

        // 中央配置
        SDL_Rect dst = {
            (winW - drawW) / 2,
            (winH - drawH) / 2,
            drawW,
            drawH
        };

        SDL_RenderCopy(renderer, texture, NULL, &dst);
        SDL_RenderPresent(renderer);
    }
};
