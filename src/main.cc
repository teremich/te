#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdio>
#include <cstdlib>
#include <editor.hpp>
#include <util.hpp>
#include <logging.hpp>

[[maybe_unused]] static void SDLCALL saveFileCallback(void *userdata, const char * const *filelist, int filter) {
    UNUSED(filter);
    if (!filelist) {
        return;
    }
    if (!filelist[0]) {
        return;
    }
    if (!*filelist[0]) {
        return;
    }
    UNUSED(userdata);
    (void)(filelist[0]);
}

[[maybe_unused]] static void SDLCALL openFileCallback(void* userdata, const char * const *filelist, int filter) {
    UNUSED(filter);
    if (!filelist) {
        return;
    }
    if (!filelist[0]) {
        return;
    }
    if (!*filelist[0]) {
        return;
    }
    UNUSED(userdata);
    UNUSED(filelist[0]);
}

void keyDown(SDL_KeyboardEvent key) {
    static_cast<void>(key);
}

bool handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_EVENT_QUIT:
                return false;
            // case SDL_EVENT_MOUSE_MOTION:
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                UNUSED(event.button);
                break;
            // case SDL_EVENT_MOUSE_BUTTON_UP:
            case SDL_EVENT_KEY_DOWN:
                keyDown(event.key);
                break;
            // case SDL_EVENT_KEY_UP:
            case SDL_EVENT_TEXT_INPUT:
                for (const char* c = event.text.text; *c != 0; c++) {
                    UNUSED(*c);
                }
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
    
}

void render(SDL_Renderer* renderer, TTF_Font* font) {
   UNUSED(renderer);
   UNUSED(font);
}

static TTF_Font* FreeMono30;
TTF_Font*& defaultFont = FreeMono30;

int main(int argc, char* argv[]) {

    Text text{};
    text.insert('H');
    text.insert('e');
    text.insert('l');
    text.insert('l');
    text.insert('o');
    text.insert(' ');
    text.insert('W');
    text.insert('o');
    text.insert('r');
    text.insert('l');
    text.insert('d');
    text.insert('!');
    text.insert('\n');
    text.insert("Hello, again!\n");

    for (char c : text) {
        printf("%c", c);
    }

    return 0;


    UNUSED(argc);
    UNUSED(argv);
    SDL_CHK(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));
    SDL_CHK(TTF_Init());
    
    for (int logLevel = SDL_LOG_CATEGORY_CUSTOM; logLevel < CUSTOM_LOG_CATEGORY_LAST; logLevel++) {
        SDL_SetLogPriority(logLevel, SDL_LOG_PRIORITY_TRACE);
    }
    SDL_SetLogPriority(CUSTOM_LOG_CATEGORY_EXPLORER, SDL_LOG_PRIORITY_INFO);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_CHK(SDL_CreateWindowAndRenderer("text editor", 1600, 900, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_TRANSPARENT, &window, &renderer));
    SDL_StartTextInput(window);
    
    FreeMono30 = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeMono.ttf", 30);
    if (!FreeMono30) {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "freemono in ptsize 30 doesn't exist\n");
        exit(1);
    }
    while (handleEvents()) {
        update();
        render(renderer, defaultFont);
    }
    TTF_CloseFont(FreeMono30);
    FreeMono30 = NULL;
    TTF_Quit();
    SDL_Quit();
    return 0;
}
