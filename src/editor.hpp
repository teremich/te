#pragma once

#include "text.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdio>

class Editor{
    List<Text> openFiles;
    ssize_t openFile = -1;
    ssize_t startLine = 0;
    const char* openFolder = nullptr;
    public:
    Editor() = default;
    Editor(List<Text>&& oFiles, const char* oFolder) {
        openFiles = std::move(oFiles);
        openFile = openFiles.size-1;
        startLine = 0;
        openFolder = oFolder;
    }
    Editor& operator=(Editor&& moveFrom) {
        openFiles = std::move(moveFrom.openFiles);
        openFile = moveFrom.openFile;
        startLine = moveFrom.startLine;
        openFolder = moveFrom.openFolder;
        return *this;
    }
    ~Editor();
    enum SpecialKey{
        DEL, BACKSPACE,
        LAST
    };
    void render(SDL_Renderer* renderer, SDL_FRect into, TTF_Font* font) const;
    void update();
    void write(const char* str);
    void write(SDL_KeyboardEvent key);
    void print() const {
        printf("%s: (%zd / %zu)\n - %p\n", openFolder, openFile, openFiles.size, &openFiles.items[openFile]);
    }
};
