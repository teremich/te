#include "editor.hpp"
#include <filesystem>
#include <logging.hpp>

void ExplorerSection::draw(SDL_Renderer* renderer, SDL_FRect dimensions, struct TTF_Font* font) const {
    const SDL_FRect smaller = {
        dimensions.x+1,
        dimensions.y+1,
        dimensions.w-2,
        dimensions.h-2,
    };

    SDL_CHK(SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255));
    SDL_CHK(SDL_RenderRect(renderer, &dimensions));
    SDL_CHK(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255));
    SDL_CHK(SDL_RenderFillRect(renderer, &smaller));

    SDL_FRect dest{smaller.x+10, smaller.y+40, 0, 0};
    SDL_Texture* texture = NULL;
    SDL_CHK(text(
        renderer,
        basePath.filename().c_str(),
        &texture,
        font,
        &dest.w,
        &dest.h
    ));
    SDL_CHK(SDL_RenderTexture(renderer, texture, NULL, &dest));
    SDL_DestroyTexture(texture);
    texture = NULL;
    dest = dest + SDL_FPoint{20, dest.h+10};
    for (const auto& entry : entries) {
        SDL_CHK(text(
            renderer,
            entry.path().filename().c_str(),
            &texture,
            font,
            &dest.w,
            &dest.h
        ));
        SDL_CHK(SDL_RenderTexture(renderer, texture, NULL, &dest));
        SDL_DestroyTexture(texture);
        texture = NULL;
        dest = dest + SDL_FPoint{0, dest.h+5};
    }
}
