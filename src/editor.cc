#include "editor.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

void TextSection::draw(SDL_Renderer* renderer) const {
    (void)renderer;
    printf("TextSection::draw()\nvisStart=%zu\n", visStart);
}

void ExplorerSection::draw(SDL_Renderer* renderer) const {
    (void)renderer;
    printf("ExplorerSection::draw()");
    for (const auto& entry : mPath) {
        printf("%s\n", entry.c_str());
    }
}
