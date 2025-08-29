#pragma once
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <libgen.h>

struct Section{
    static const enum class Type{
        UNDEFINED,
        TEXTFILE,
        EXPLORER,
        LAST,
    } type = Section::Type::UNDEFINED;
    Section() = default;
    ~Section() = default;
    virtual void draw(struct SDL_Renderer* renderer) const = 0;
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
    void draw(struct SDL_Renderer* renderer) const override;
    // NULL opens a tmpfile
    void open(const char* file) {
        if (file) {
            fileHandle = fopen(file, "w+");
        } else {
            fileHandle = tmpfile();
            freopen(NULL, "w+", fileHandle);
        }
        assert(fileHandle);
        fseek(fileHandle, 0, SEEK_END);
        fileSize = ftell(fileHandle);
        rewind(fileHandle);
        content = static_cast<char*>(malloc(fileSize+1024));
        if (!content) {
            exit(-ENOMEM);
        }
        bufferSize = 1024;
        fread(content+cursor+bufferSize, fileSize, 1, fileHandle);
    }
    void write(char c) {
        assert(fileHandle);
        if (!bufferSize) {
            grow();
        }
        content[cursor] = c;
        bufferSize--;
        fileSize++;
        cursor++;
    }
    void save() {
        assert(fileHandle);
        flush();
        fflush(fileHandle);
    }
    void saveas(const char* newFile) {
        if (fileHandle) {
            fclose(fileHandle);
        }
        fileHandle = fopen(newFile, "w+");
        save();
    }
    struct movement{
        bool forward  : 1;
        bool wordWise : 1;
        bool lineWise : 1;
        bool full     : 1;
        bool select   : 1;
        operator uint8_t() {
            return *std::bit_cast<uint8_t*>(this);
        }
    };
    void moveRel(movement to) {
        switch(to) {
            case 0b0000: // LEFT -> one char back
            if (cursor == 0) {
                    return;
                }
                cursor--;
                content[cursor+bufferSize] = content[cursor];
                break;
                case 0b1000: // RIGHT -> one char forward
                if (cursor == fileSize) {
                    return;
                }
                content[cursor] = content[cursor+bufferSize];
                cursor++;
                break;
            case 0b0100: // CTRL+LEFT -> one word back
            while (cursor && !isWordBreak(content[cursor-1], content[cursor])) {
                    cursor--;
                    content[cursor+bufferSize] = content[cursor];
                }
                break;
                case 0b1100: // CTRL+RIGHT -> one word forward
                while (cursor < fileSize && !isWordBreak(content[cursor+1], content[cursor])) {
                    content[cursor] = content[cursor+bufferSize];
                    cursor++;
                }
                break;
                case 0b0010: // UP -> one line back
                {
                    int32_t col = 0;
                    int32_t lineSize = -1;
                    bool end = false;
                    bool foundUpperLine = false;
                    while (cursor && !end) {
                        cursor--;
                        content[cursor+bufferSize] = content[cursor];
                        if (content[cursor] == '\n') {
                            end = foundUpperLine;
                            foundUpperLine = true;
                        }
                        if (foundUpperLine) {
                            lineSize++;
                        } else {
                            col++;
                        }
                    }
                    if (lineSize < 0) {
                        return;
                    }
                    if (!end) {
                        assert(cursor == 0);
                        int32_t finalCol = std::min(col, lineSize);
                        assert(finalCol >= 0);
                        moveAbs(finalCol+cursor);
                    }
                }
                break;
            case 0b1010: // DOWN -> one line forward
            break;
            case 0b0001: // HOME -> beginning of line
            break;
            case 0b1001: // END -> end of line
            break;
            case 0b0101: // CTRL+HOME -> beginning of file
            break;
            case 0b1101: // CTRL+END -> end of file
            break;
            default:
            return;
        }
    }
    void moveAbs(size_t to) {
        if (to > cursor) {
            // cursor moves right
            std::memmove(content+cursor, content+cursor+bufferSize, to-cursor);
        } else if (to < cursor) {
            // cursor moves left
            std::memmove(content+to+bufferSize, content+to, cursor-to);
        }
        cursor = to;
    }
    ~TextSection() {
        if (fileHandle) {
            flush();
            fclose(fileHandle);
        }
        if (content) {
            free(content);
            content = nullptr;
        }
    }
    private:
    FILE* fileHandle = NULL;
    size_t cursor = 0;
    size_t visStart = 0;
    size_t fileSize = 0;
    size_t bufferSize = 0;
    [[maybe_unused]] ssize_t selectionStart = -1;
    char* content = nullptr;
    void grow(size_t newSize = 1024) {
        const auto oldContent = content;
        content = static_cast<char*>(realloc(content, fileSize+newSize));
        if (!content) {
            content = oldContent;
            exit(-ENOMEM);
        }
        bufferSize = newSize;
    }
    void flush() {
        rewind(fileHandle);
        fwrite(content, cursor, 1, fileHandle);
        fwrite(content+cursor+bufferSize, fileSize-cursor, 1, fileHandle);
        grow();
    }
    // content layout
    // 
    // 0                            cursor  cursor+bufferSize bufferSize+fileSize
    // [ beginning of file to cursor ] [ bufferSize ] [ end of file ]
};

class ExplorerSection : public Section{
    public:
    static const Section::Type type = Section::Type::EXPLORER;
    ExplorerSection() = default;
    ~ExplorerSection() = default;
    explicit ExplorerSection(const char* path) : 
    mPath(path) {}
    void draw(struct SDL_Renderer* renderer) const override;
    private:
    std::filesystem::path mPath;
};

struct EditorState{
    TextSection text;
    ExplorerSection explorer;
    explicit EditorState(const char* filepath) {
        if (!filepath) {
            return;
        }
        char* path = static_cast<char*>(calloc(1, std::strlen(filepath) + 1));
        if (!path) {
            exit(-ENOMEM);
        }
        strcpy(path, filepath);
        explorer = ExplorerSection(dirname(path));
        free(path);
        if (std::filesystem::is_regular_file(filepath)) {
            text.open(filepath);
        }
        text.open(nullptr);
    }
    void draw(struct SDL_Renderer* renderer) {
        text.draw(renderer);
        explorer.draw(renderer);
    }
};

