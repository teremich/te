#include "editor.hpp"

SDL_Rect drawLine(const char* buffer, int len, const SDL_FRect& into, TTF_Font* font, SDL_Renderer* renderer) {
    SDL_Surface* renderedStrip = TTF_RenderText_Blended(font, buffer, len, SDL_Color{255, 255, 255, 255});
    auto screen = SDL_GetWindowSurface(SDL_GetRenderWindow(renderer));
    SDL_Rect srcrect;
    SDL_CHK(SDL_GetSurfaceClipRect(renderedStrip, &srcrect));
    if (srcrect.w > into.w) {
        srcrect.w = into.w;
    }
    if (srcrect.h > into.h) {
        srcrect.h = into.h;
    }
    SDL_CHK(SDL_SetSurfaceClipRect(renderedStrip, &srcrect));
    SDL_Rect dstrect{(int)into.x, (int)into.y, srcrect.w, srcrect.h};
    SDL_CHK(SDL_BlitSurface(renderedStrip, NULL, screen, &dstrect));
    SDL_DestroySurface(renderedStrip);
    return srcrect;
}

static void renderText(SDL_Renderer* renderer, SDL_FRect into, Text::Iterator& begin, const Text::Iterator& end, TTF_Font* font) {
    static char buffer[16];
    int i = 0;
    SDL_FPoint orig{into.x, into.w};
    while (into.h > 0) {
        if (begin == end) {
            if (i) {
                drawLine(buffer, i, into, font, renderer);
            }
            return;
        }
        if (*begin == '\n') {
            SDL_Rect drawn = drawLine(buffer, i, into, font, renderer);
            ++begin;
            into.y += drawn.h;
            into.h -= drawn.h;
            into.x = orig.x;
            into.w = orig.y;
            i = 0;
            continue;
        }
        if (i == SDL_arraysize(buffer)) {
            SDL_Rect drawn = drawLine(buffer, i, into, font, renderer);
            into.x += drawn.w;
            into.w -= drawn.w;
            i = 0;
        }
        buffer[i++] = *begin;
        ++begin;
    }
    // while (begin != end && into.h > 0) {
    //     for (i = 0; i < 16; i++) {
    //         if (begin == end || *begin == '\n') {
    //             break;
    //         }
    //         buffer[i] = *begin;
    //         ++begin;
    //     }
    //     if (i) {
    
    //     }
    // }
}

void Editor::render(SDL_Renderer* renderer, SDL_FRect into, TTF_Font* font) const {
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderFillRect(renderer, &into);
    if (openFile >= 0) {
        auto [begin, end] = openFiles.items[openFile].getView(startLine, 50);
        renderText(renderer, into, begin, end, font);
    }
}

Editor::~Editor() = default;
