#include "editor.hpp"

SDL_FPoint drawLine(const char* buffer, int len, const SDL_FRect& into, TTF_Font* font, SDL_Renderer* renderer) {
    if (!len) {
        return {0, (float)TTF_GetFontHeight(font)};
    }
    SDL_Surface* renderedStrip = TTF_RenderText_Blended(font, buffer, len, SDL_Color{255, 255, 255, 255});
    // SDL_CHK(SDL_GetSurfaceClipRect(renderedStrip, &srcrect));
    // if (srcrect.w > into.w) {
    //     srcrect.w = into.w;
    // }
    // if (srcrect.h > into.h) {
    //     srcrect.h = into.h;
    // }
    // SDL_CHK(SDL_SetSurfaceClipRect(renderedStrip, &srcrect));
    auto stripTexture = SDL_CreateTextureFromSurface(renderer, renderedStrip);
    float width, height;
    SDL_GetTextureSize(stripTexture, &width, &height);
    SDL_FRect srcrect = {0, 0, width, height};
    SDL_FRect dstrect{into.x, into.y, srcrect.w, srcrect.h};
    SDL_CHK(SDL_RenderTexture(renderer, stripTexture, &srcrect, &dstrect));
    SDL_DestroySurface(renderedStrip);
    return {srcrect.w, srcrect.h};
}

void drawCursor(const SDL_FRect& into, SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_FRect cursor = {into.x, into.y, 2, 30};
    SDL_RenderFillRect(renderer, &cursor);
}

static void renderText(SDL_Renderer* renderer, SDL_FRect into, Text::Iterator& begin, const Text::Iterator& end, TTF_Font* font) {
    static char buffer[16];
    int i = 0;
    SDL_FPoint orig{into.x, into.w};
    while (into.h > 0) {
        if (begin.cursorPos == begin.pos) {
            SDL_FPoint drawn = drawLine(buffer, i, into, font, renderer);
            into.x += drawn.x;
            into.w -= drawn.x;
            i = 0;
            drawCursor(into, renderer);
        }
        if (begin == end) {
            drawLine(buffer, i, into, font, renderer);
            return;
        }
        if (*begin == '\n') {
            SDL_FPoint drawn = drawLine(buffer, i, into, font, renderer);
            ++begin;
            into.y += drawn.y;
            into.h -= drawn.y;
            into.x = orig.x;
            into.w = orig.y;
            i = 0;
            continue;
        }
        if (i == SDL_arraysize(buffer)) {
            SDL_FPoint drawn = drawLine(buffer, i, into, font, renderer);
            into.x += drawn.x;
            into.w -= drawn.x;
            i = 0;
        }
        buffer[i++] = *begin;
        ++begin;
    }
}

void Editor::render(SDL_Renderer* renderer, SDL_FRect into, TTF_Font* font) const {
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderFillRect(renderer, &into);
    ssize_t maxLines = into.h / TTF_GetFontHeight(font);
    if (openFile >= 0) {
        auto [begin, end] = openFiles.items[openFile].getView(startLine, maxLines);
        renderText(renderer, {into.x+60, into.y+10, into.w-70, into.h-20}, begin, end, font);
    }
}

void Editor::update() {
    if (openFile < 0) {
        return;
    }
    // auto keyboard = SDL_GetKeyboardState(NULL);
    // bool ctrl = SDL_GetModState() & SDL_KMOD_CTRL;
    // if (keyboard[SDL_SCANCODE_UP]) {
    //     openFiles.items[openFile].up();
    //     return;
    // }
    // if (keyboard[SDL_SCANCODE_DOWN]) {
    //     openFiles.items[openFile].down();
    //     return;
    // }
    // if (keyboard[SDL_SCANCODE_LEFT]) {
    //     openFiles.items[openFile].left(ctrl);
    //     return;
    // }
    // if (keyboard[SDL_SCANCODE_RIGHT]) {
    //     openFiles.items[openFile].right(ctrl);
    //     return;
    // }
    // if (keyboard[SDL_SCANCODE_HOME]) {
    //     ctrl ? openFiles.items[openFile].beginning() : openFiles.items[openFile].home();
    //     return;
    // }
    // if (keyboard[SDL_SCANCODE_END]) {
    //     ctrl ? openFiles.items[openFile].ending() : openFiles.items[openFile].ende();
    //     return;
    // }
}

void Editor::write(const char* str) {
    if (openFile < 0) {
        return;
    }
    openFiles.items[openFile].insert(str);
}

void Editor::write(SDL_KeyboardEvent key) {
    if (openFile < 0) {
        return;
    }
    const bool ctrl = key.mod & SDL_KMOD_CTRL;
    switch(key.scancode) {
        case SDL_SCANCODE_DELETE:
            openFiles.items[openFile].del();
            break;
        case SDL_SCANCODE_BACKSPACE:
            openFiles.items[openFile].backspace();
            break;
        case SDL_SCANCODE_RETURN:
            openFiles.items[openFile].insert('\n');
            break;
        case SDL_SCANCODE_UP:
            openFiles.items[openFile].up();
            break;
        case SDL_SCANCODE_DOWN:
            openFiles.items[openFile].down();
            break;
        case SDL_SCANCODE_LEFT:
            openFiles.items[openFile].left(ctrl);
            break;
        case SDL_SCANCODE_RIGHT:
            openFiles.items[openFile].right(ctrl);
            break;
        case SDL_SCANCODE_HOME:
            ctrl ? openFiles.items[openFile].beginning() : openFiles.items[openFile].home();
            break;
        case SDL_SCANCODE_END:
            ctrl ? openFiles.items[openFile].ending() : openFiles.items[openFile].ende();
            break;
        default:
            // SDL_LogCritical(CUSTOM_LOG_CATEGORY_INPUT, "didn't handle special key %u\n", key);
            return;
    }
}

Editor::~Editor() = default;
