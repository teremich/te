#include "editor.hpp"

void TextSection::draw(SDL_Renderer* renderer, SDL_FRect dimensions, TTF_Font* font = defaultFont) const {
    SDL_Texture* texture = NULL;
    SDL_FPoint pos = {50, 50};
    SDL_FRect dest = dimensions + pos;
    SDL_CHK(text(
        renderer,
        content,
        &texture,
        font,
        &dest.w,
        &dest.h,
        cursor,
        {0, 255, 0, 255}
    ));
    SDL_CHK(SDL_RenderTexture(renderer, texture, NULL, &dest));
    SDL_DestroyTexture(texture);
}