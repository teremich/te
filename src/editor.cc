#include "editor.hpp"

static void drawLine(const char* buffer, int len, SDL_FRect& into, TTF_Font* font, SDL_Renderer* renderer) {
    SDL_Surface* renderedStrip = TTF_RenderText_Blended(font, buffer, len, SDL_Color{255, 255, 255, 255});
    auto dst = SDL_GetWindowSurface(SDL_GetRenderWindow(renderer));
    // SDL_Rect srcrect;
    // SDL_GetSurfaceClipRect(renderedStrip, &srcrect);
    SDL_Rect dstrect{(int)into.x, (int)into.y, 0, 0};
    SDL_BlitSurface(renderedStrip, NULL, dst, &dstrect);
}

static void renderText(SDL_Renderer* renderer, SDL_FRect into, Text::Iterator& begin, const Text::Iterator& end, TTF_Font* font) {
    static char buffer[16];
    int i = 0;
    while (into.h > 0) {
        if (begin == end) {
            if (i) {
                drawLine(buffer, i, into, font, renderer);
            }
            return;
        }
        if (*begin == '\n') {
            drawLine(buffer, i, into, font, renderer);
            ++begin;
            into.y += 30;
            into.h -= 30;
            continue;
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
