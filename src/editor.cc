#include "editor.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <logging.hpp>
#include <util.hpp>

bool text(
    SDL_Renderer* renderer,
    const char* content,
    SDL_Texture** out,
    TTF_Font* font,
    float* width,
    float* height,
    size_t length,
    SDL_Color color
) {
    const auto surface = TTF_RenderText_Blended(font, content, length, color);
    if (!surface) {
        return false;
    }
    *width = static_cast<float>(surface->w);
    *height = static_cast<float>(surface->h);
    auto texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) {
        return false;
    }
    *out = texture;
    return true;
}


