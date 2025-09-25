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
    Editor(List<Text>&& oFiles, ssize_t oFile, ssize_t sLine, const char* oFolder) {
        openFiles = std::move(oFiles);
        openFile = oFile;
        startLine = sLine;
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
    void render(SDL_Renderer* renderer, SDL_FRect into, TTF_Font* font) const;
    void update();
    void print() const {
        printf("%s: (%zd / %zu)\n - %p\n", openFolder, openFile, openFiles.size, &openFiles.items[openFile]);
    }
};
