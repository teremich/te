#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdio>
#include <cstdlib>
#include "editor.hpp"

// #ifdef DEBUG
#ifdef SDL_CHK
#error "redefining SDL_CHK"
#endif // SDL_CHK

#define SDL_CHK(X) if (!(X)) {fprintf(stderr, "Error at %s:%d: %s", __FILE__, __LINE__, SDL_GetError()); exit(1);}
// #endif // DEBUG

bool handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_EVENT_QUIT:
                return false;
            // case SDL_EVENT_MOUSE_MOTION:
            // case SDL_EVENT_MOUSE_BUTTON_DOWN:
            // case SDL_EVENT_MOUSE_BUTTON_UP:
            // case SDL_EVENT_KEY_DOWN:
            // case SDL_EVENT_KEY_UP:
            // case SDL_EVENT_WINDOW_RESIZED:
            // case SDL_EVENT_WINDOW_RESTORED:
            // case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            // case SDL_EVENT_DROP_FILE:
            // case SDL_EVENT_MOUSE_WHEEL:
            // case SDL_EVENT_TEXT_INPUT:
            // case SDL_EVENT_TEXT_EDITING:
            default:
                break;
        }
    }
    return true;
}

void update() {
    // TODO
}

void render(SDL_Renderer* renderer, EditorState& state) {
    SDL_CHK(SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255));
    SDL_CHK(SDL_RenderClear(renderer));

    static auto FreeMono38 = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeMono.ttf", 38);
    if (!FreeMono38) {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "freemono38 doesn't exist\n");
        exit(1);
    }
    auto surface = TTF_RenderText_Blended(FreeMono38, "text", 0, {220, 220, 220, 255});
    if (!surface) {
        return;
    }
    SDL_FRect dest{50, 50,
        static_cast<float>(surface->w),
        static_cast<float>(surface->h)
    };

    auto texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    surface = NULL;
    if (!texture) {
        return;
    }

    state.draw(renderer);

    SDL_CHK(SDL_RenderTexture(renderer, texture, NULL, &dest));
    SDL_CHK(SDL_RenderPresent(renderer));
    SDL_DestroyTexture(texture);
}

int main(int argc, char* args[]) {
    SDL_CHK(SDL_Init(SDL_INIT_VIDEO));
    SDL_CHK(TTF_Init());
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_CHK(SDL_CreateWindowAndRenderer("text editor", 1600, 900, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_TRANSPARENT, &window, &renderer));
    const char* path = ".";
    if (argc > 1) {
        path = args[1];
    }
    EditorState state{path};
    while (handleEvents()) {
        update();
        render(renderer, state);
    }
    TTF_Quit();
    SDL_Quit();
    return 0;
}
