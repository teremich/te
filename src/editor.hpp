#pragma once
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <cassert>
#include <cstring>
#include <vector>
#include <SDL3/SDL.h>
#include <util.hpp>
#include <logging.hpp>

struct Section{
    static const enum class Type{
        UNDEFINED,
        TEXTFILE,
        EXPLORER,
        LAST,
    } type = Section::Type::UNDEFINED;
    Section() = default;
    ~Section() = default;
    // dimensions -> {xy=offset, wh=available space}
    virtual void draw(struct SDL_Renderer* renderer, SDL_FRect dimensions, struct TTF_Font* font) const = 0;
};

static inline bool isWordBreak(char to, char from) {
    const bool fromIsUpper = 'A' <= from && from <= 'Z';
    const bool fromIsLower = 'a' <= from && from <= 'z';
    const bool fromIsNumber = '0' <= from && from <= '9';
    const bool fromIsAlphaNum = fromIsUpper || fromIsLower || fromIsNumber;
    const bool toIsUpper = 'A' <= to && to <= 'Z';
    const bool toIsLower = 'a' <= to && to <= 'z';
    const bool toIsNumber = '0' <= to && to <= '9';
    const bool toIsAlphaNum = toIsUpper || toIsLower || toIsNumber;
    const bool fromIsWhiteSpace =
    0x20 == from || // SPACE
    0x9 == from || // TAB
    0xa == from || // LINE FEED
    0xb == from || // LINE TABULATION
    0xc == from || // FORM FEED
    0xd == from; // CARRIAGE RETURN
    const bool toIsWhiteSpace =
    0x20 == to || // SPACE
    0x9 == to || // TAB
    0xa == to || // LINE FEED
    0xb == to || // LINE TABULATION
    0xc == to || // FORM FEED
    0xd == to; // CARRIAGE RETURN
    const bool fromIsSpecial = !(fromIsAlphaNum || fromIsWhiteSpace);
    const bool toIsSpecial = !(toIsAlphaNum || toIsWhiteSpace);
    return !fromIsWhiteSpace && (fromIsSpecial != toIsSpecial || fromIsAlphaNum != toIsAlphaNum);

    //                     FROM IS ALPHA NUM | FROM IS WHITE SPACE | FROM IS SPECIAL CHAR
    // TO IS ALPHA NUM          false                   false               true
    // TO IS WHITE SPACE        true                    false               true
    // TO IS SPECIAL CHAR       true                    false               false

}

class TextSection : public Section{
    public:
    static const Section::Type type = Section::Type::TEXTFILE;
    void draw(struct SDL_Renderer* renderer, SDL_FRect dimensions, struct TTF_Font* font) const override;
    // NULL opens a tmpfile
    void open(const char* file);
    void write(char c);
    enum{
        MOVEMENT_forward  = 1<<(0), // 0x01
        MOVEMENT_wordWise = 1<<(1), // 0x02
        MOVEMENT_lineWise = 1<<(2), // 0x04
        MOVEMENT_full     = 1<<(3), // 0x08
        MOVEMENT_select   = 1<<(4), // 0x10
    };
    typedef uint8_t movement;
    void del(movement to);
    // TODO
    // void delSelection();
    void close();
    void save() const;
    void saveas(const char* newFile);
    void moveRel(movement to);
    void moveAbs(size_t to);
    ~TextSection();
    TextSection();
    TextSection& operator=(TextSection&& moveFrom) {
        std::memcpy(
            reinterpret_cast<void*>(this),
            reinterpret_cast<void*>(&moveFrom),
            sizeof(*this)
        );
        moveFrom.content = nullptr;
        moveFrom.fileHandle = NULL;
        return *this;
    }
    private:
    FILE* fileHandle = NULL;
    size_t cursor = 0;
    SDL_FPoint visStart = {0, 0};
    size_t fileSize = 0;
    size_t bufferSize = 0;
    [[maybe_unused]] ssize_t selectionStart = -1;
    // content layout
    // 0                            cursor  cursor+bufferSize bufferSize+fileSize
    // [ beginning of file to cursor ] [ bufferSize ] [ end of file ]
    char* content = nullptr;
    void grow(size_t newSize = 1024);
    void flush() const;
    void drawFileText(SDL_Renderer* renderer, SDL_FRect dimensions, TTF_Font* font) const;
    void drawCursor(SDL_Renderer* renderer, SDL_FRect dimensions) const;
};

class ExplorerSection : public Section{
    public:
    static const Section::Type type = Section::Type::EXPLORER;
    ExplorerSection() = default;
    ~ExplorerSection() = default;
    explicit ExplorerSection(std::filesystem::path absolute);
    void draw(struct SDL_Renderer* renderer, SDL_FRect dimensions, struct TTF_Font* font) const override;
    private:
    std::filesystem::path basePath;
    std::vector<std::filesystem::directory_entry> entries;
};

struct EditorState{
    TextSection text;
    ExplorerSection explorer;
    EditorState() = default;
    explicit EditorState(const char* filepath) {
        if (!filepath) {
            return;
        }
        char* path = static_cast<char*>(calloc(1, std::strlen(filepath) + 1));
        if (!path) {
            exit(-ENOMEM);
        }
        strcpy(path, filepath);
        const auto parent = std::filesystem::absolute(std::filesystem::path(path)).parent_path();
        explorer = ExplorerSection(parent);
        free(path);
        if (std::filesystem::is_regular_file(filepath)) {
            text.open(filepath);
        }
        text.open(nullptr);
    }
    void draw(SDL_Renderer* renderer, struct TTF_Font* font) {
        int width, height;
        SDL_CHK(SDL_GetCurrentRenderOutputSize(renderer, &width, &height));
        explorer.draw(renderer, {
            0, 0, 420,
            static_cast<float>(height)
        }, font);
        text.draw(renderer, {
            420, 0, 
            static_cast<float>(width),
            static_cast<float>(height)
        }, font);
    }
};

extern struct TTF_Font*& defaultFont;

bool text(
    SDL_Renderer* renderer,
    const char* content,
    SDL_Texture** out,
    TTF_Font* font,
    float* width = nullptr,
    float* height = nullptr,
    size_t length = 0,
    SDL_Color color = {255, 255, 255, 255}
);

static inline SDL_FRect operator+(const SDL_FRect& lhs, const SDL_FPoint& rhs) {
    return {.x=lhs.x+rhs.x, .y=lhs.y+rhs.y, .w=lhs.w, .h=lhs.h};
}
