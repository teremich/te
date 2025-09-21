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

using
    std::filesystem::directory_entry,
    std::filesystem::directory_iterator
;

static std::vector<directory_entry>::iterator findSpot(std::vector<directory_entry>& entries, const directory_entry& entry) {
    for (std::vector<directory_entry>::iterator begin = entries.begin(); begin != entries.end(); ++begin) {
        if (entry.is_directory() && !begin->is_directory()) {
            return begin;
        } else if (!entry.is_directory() && begin->is_directory()) {
            continue;
        }
        if (strcmp(begin->path().c_str(), entry.path().c_str()) > 0) {
            return begin;
        }
    }
    return entries.end();
}

ExplorerSection::ExplorerSection(std::filesystem::path absolute) : basePath(absolute) {
    for (const auto& entry : directory_iterator(basePath)) {
        entries.insert(findSpot(entries, entry), entry);
    }
}
