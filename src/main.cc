#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdio>
#include <cstdlib>
#include <editor.hpp>
#include <util.hpp>
#include <logging.hpp>

Editor editor;

void keyDown(SDL_KeyboardEvent key) {
    editor.write(key);
}

bool handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_EVENT_QUIT:
                return false;
            // case SDL_EVENT_MOUSE_MOTION:
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                editor.buttonDown(event.button);
                break;
            // case SDL_EVENT_MOUSE_BUTTON_UP:
            case SDL_EVENT_MOUSE_WHEEL:
                editor.scroll(event.wheel);
                break;
            case SDL_EVENT_KEY_DOWN:
                keyDown(event.key);
                break;
            // case SDL_EVENT_KEY_UP:
            case SDL_EVENT_TEXT_INPUT:
                editor.write(event.text.text);
                break;
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
    editor.update();
}

void render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    int width, height;
    SDL_GetWindowSize(SDL_GetRenderWindow(renderer), &width, &height);
    editor.render(
        renderer,
        SDL_FRect{0, 0, (float)width, (float)height}
    );
    SDL_RenderPresent(renderer);
}

static TTF_Font* FreeMono30;
TTF_Font*& selectedFont = FreeMono30;

int main(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);
    SDL_CHK(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));
    SDL_CHK(TTF_Init());
    // I can even add comments :D
    for (int logLevel = SDL_LOG_CATEGORY_CUSTOM; logLevel < CUSTOM_LOG_CATEGORY_LAST; logLevel++) {
        SDL_SetLogPriority(logLevel, SDL_LOG_PRIORITY_TRACE);
    }
    SDL_SetLogPriority(CUSTOM_LOG_CATEGORY_EXPLORER, SDL_LOG_PRIORITY_INFO);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_CHK(SDL_CreateWindowAndRenderer("text editor", 1600, 900, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_TRANSPARENT, &window, &renderer));
    SDL_StartTextInput(window);
    
    FreeMono30 = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeMono.ttf", 30);
    if (!FreeMono30) {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "freemono in ptsize 30 doesn't exist\n");
        exit(1);
    }
    editor = Editor(selectedFont);
    editor.open("src/main.cc");
    while (handleEvents()) {
        // Timer t("=================================\nframe");
        update();
        render(renderer);
    }
    TTF_CloseFont(FreeMono30);
    FreeMono30 = NULL;
    
    TTF_Quit();
    SDL_Quit();
    return 0;
}
