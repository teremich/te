#pragma once

#include "text.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdio>
#include <vector>


class Editor{
    struct OpenFile{
        size_t index{0};
        mutable ssize_t startLine{0};
        std::vector<ssize_t> newLineIndices{};
        ssize_t numLinesBeforeCursor{0};
        ssize_t inlineOffset{-1};
    };
    List<Text> files{};
    OpenFile currentFile;
    std::vector<std::string> filenames{};
    const char* folder{nullptr};
    public:
    Editor() {
        updateInlineOffset();
    };
    // Editor(List<Text>&& oFiles, const char* oFolder) {
    //     files = std::move(oFiles);
    //     currentFile = {files.size-1, 0, {}, -1, 0};
    //     folder = oFolder;
    //     for (auto it = std::begin(files.items[currentFile.index]); it != std::end(files.items[currentFile.index]); ++it) {
    //         if (*it == '\n') {
    //             currentFile.newLineIndices.push_back(it.pos);
    //         }
    //     }
    // }
    Editor& operator=(Editor&& moveFrom) {
        files = std::move(moveFrom.files);
        moveFrom.filenames.swap(filenames);
        currentFile = moveFrom.currentFile;
        folder = moveFrom.folder;
        return *this;
    }
    ~Editor();
    enum SpecialKey{
        DEL, BACKSPACE,
        LAST
    };
    size_t open(const char* relativeFilePath);
    void close(size_t index);
    void switchTo(size_t index);
    void render(SDL_Renderer* renderer, SDL_FRect into, TTF_Font* font) const;
    void update();
    void write(const char* str);
    void write(SDL_KeyboardEvent key);
    void updateInlineOffset();
    void saveAs(const char* filename) {
        filenames.at(currentFile.index) = filename;
        files.items[currentFile.index].save(filename);
    }
    void print() const {
        printf("%s: (%zd / %zu)\n - %s\n", folder, currentFile.index, files.size, filenames[currentFile.index].c_str());
    }
};
