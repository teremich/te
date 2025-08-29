#include "editor.hpp"
#include <filesystem>
#include <logging.hpp>

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
