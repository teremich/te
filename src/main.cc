#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdio>
#include <cstdlib>
#include "SDL3/SDL_events.h"
#include "editor.hpp"
#include <util.hpp>
#include <logging.hpp>

static EditorState state;

static void SDLCALL saveFileCallback(void *userdata, const char * const *filelist, int filter) {
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
    TextSection* text = std::bit_cast<TextSection*>(userdata);
    text->saveas(filelist[0]);
}

static void SDLCALL openFileCallback(void* userdata, const char * const *filelist, int filter) {
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
    TextSection* text = std::bit_cast<TextSection*>(userdata);
    text->open(filelist[0]);
}

void keyDown(SDL_KeyboardEvent key) {
    TextSection::movement mvmnt =
        TextSection::MOVEMENT_wordWise * (bool)(SDL_GetModState() & (SDL_KMOD_CTRL)) +
        TextSection::MOVEMENT_select * (bool)(SDL_GetModState() & (SDL_KMOD_SHIFT));
    switch(key.key) {
        case SDLK_BACKSPACE:
            state.text.del(mvmnt);
            return;
        case SDLK_DELETE:
            mvmnt += TextSection::MOVEMENT_forward;
            state.text.del(mvmnt);
            return;
        case SDLK_DOWN:
            mvmnt += TextSection::MOVEMENT_forward;
            [[fallthrough]];
        case SDLK_UP:
            mvmnt += TextSection::MOVEMENT_lineWise;
            state.text.moveRel(mvmnt);
            return;
        case SDLK_RIGHT:
            mvmnt += TextSection::MOVEMENT_forward;
            [[fallthrough]];
        case SDLK_LEFT:
            state.text.moveRel(mvmnt);
            return;
        case SDLK_END:
            mvmnt += TextSection::MOVEMENT_forward;
            [[fallthrough]];
        case SDLK_HOME:
            mvmnt += TextSection::MOVEMENT_full;
            state.text.moveRel(mvmnt);
            return;
        case SDLK_RETURN:
            state.text.write('\n');
            return;
        default:
            break;
    }
    SDL_Event tmp{.key = key};
    if (SDLK_S == key.key && (bool)(SDL_GetModState() & (SDL_KMOD_LCTRL))) {
        if(!state.text.hasOpenFile() || (bool)(SDL_GetModState() & (SDL_KMOD_SHIFT))) {
            SDL_ShowSaveFileDialog(
                saveFileCallback,
                &state.text,
                SDL_GetWindowFromEvent(&tmp),
                NULL,
                0,
                "."
            );
        } else {
            state.text.save();
        }
        return;
    }
    if (SDLK_O == key.key && (bool)(SDL_GetModState() & (SDL_KMOD_LCTRL))) {
        SDL_ShowOpenFileDialog(openFileCallback, &state.text, SDL_GetWindowFromEvent(&tmp), NULL, 0, NULL, 1);
        return;
    }
}

bool handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_EVENT_QUIT:
                return false;
            // case SDL_EVENT_MOUSE_MOTION:
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                state.text.moveAbs(event.button.x, event.button.y);
                break;
            // case SDL_EVENT_MOUSE_BUTTON_UP:
            case SDL_EVENT_KEY_DOWN:
                keyDown(event.key);
                break;
            // case SDL_EVENT_KEY_UP:
            case SDL_EVENT_TEXT_INPUT:
                for (const char* c = event.text.text; *c != 0; c++) {
                    state.text.write(*c);
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
    // may be useless
}

void render(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_CHK(SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255));
    SDL_CHK(SDL_RenderClear(renderer));

    state.draw(renderer, font);

    SDL_CHK(SDL_RenderPresent(renderer));
}

static TTF_Font* FreeMono30;
TTF_Font*& defaultFont = FreeMono30;

int main(int argc, char* args[]) {
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
    
    const char* path = ".";
    if (argc > 1) {
        path = args[1];
    }
    state = EditorState{path};
    while (handleEvents()) {
        update();
        render(renderer, defaultFont);
    }
    state.text.close();
    TTF_CloseFont(FreeMono30);
    FreeMono30 = NULL;
    TTF_Quit();
    SDL_Quit();
    return 0;
}
