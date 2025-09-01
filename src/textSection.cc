#include "editor.hpp"
#include "logging.hpp"
#include <cstddef>
#include <util.hpp>
#include <cstdio>
#include <vector>
#include <algorithm>

[[maybe_unused]] static inline size_t myfread(
    void *__restrict __ptr,
    size_t __size,
    size_t __n,
    FILE *__restrict __stream
) {
    size_t read = fread(__ptr, __size, __n, __stream);
    SDL_LogTrace(CUSTOM_LOG_CATEGORY_INPUT, "fread(%p, %zu, %zu, %p) -> %zu\n", __ptr, __size, __n, __stream, read);
    return read;
}
    
[[maybe_unused]] static inline size_t myfwrite(
    const void *__restrict __ptr,
    size_t __size,
    size_t __n,
    FILE *__restrict __s
) {
    size_t written = fwrite(__ptr, __size, __n, __s);
    SDL_LogTrace(CUSTOM_LOG_CATEGORY_INPUT, "fwrite(%p, %zu, %zu, %p) -> %zu\n", __ptr, __size, __n, __s, written);
    return written;
}
        
[[maybe_unused]] static inline FILE *myfopen(
    const char *__restrict __filename,
    const char *__restrict __modes
) {
    FILE* file = fopen(__filename, __modes);
    SDL_LogTrace(CUSTOM_LOG_CATEGORY_INPUT, "fopen(%s, %s) -> %p\n", __filename, __modes, file);
    return file;
}
            
[[maybe_unused]] static inline int myfclose(
    FILE *__stream
) {
    int ret = fclose(__stream);
    SDL_LogTrace(CUSTOM_LOG_CATEGORY_INPUT, "fclose(%p) -> %d\n", __stream, ret);
    return ret;
}

[[maybe_unused]] static inline FILE* mytmpfile() {
    FILE* file = tmpfile();
    SDL_LogTrace(CUSTOM_LOG_CATEGORY_INPUT, "tmpfile() -> %p\n", file);
    return file;
}

[[maybe_unused]] static inline int myfseek(
    FILE *__stream,
    long __off,
    int __whence
) {
    int ret = fseek(__stream, __off, __whence);
    SDL_LogTrace(CUSTOM_LOG_CATEGORY_INPUT, "fseek(%p, %ld, %d) -> %d\n", __stream, __off, __whence, ret);
    return ret;
}
[[maybe_unused]] static inline long myftell(FILE*__stream) {
    long ret = ftell(__stream);
    SDL_LogTrace(CUSTOM_LOG_CATEGORY_INPUT, "ftell(%p) -> %ld\n", __stream, ret);
    return ret;
}
[[maybe_unused]] static inline void myrewind(FILE *__stream) {
    rewind(__stream);
    SDL_LogTrace(CUSTOM_LOG_CATEGORY_INPUT, "rewind(%p) -> void\n", __stream);
    return;
}

[[maybe_unused]] static inline FILE* myfreopen(
    const char *__restrict __filename,
    const char *__restrict __modes,
    FILE *__restrict __stream
) {
    FILE* file = freopen(__filename, __modes, __stream);
    SDL_LogTrace(CUSTOM_LOG_CATEGORY_INPUT, "freopen(%s, %s, %p) -> %p\n", __filename, __modes, __stream, file);
    return file;
}

#ifdef LOG_IO_OPS
#define fread myfread
#define fwrite myfwrite
#define fopen myfopen
#define fclose myfclose
#define tmpfile mytmpfile
#define fseek myfseek
#define ftell myftell
#define rewind myrewind
#define freopen myfreopen
#endif

static void drawTextChunk(
    const char* chunk,
    size_t length,
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    TTF_Font* font,
    SDL_FRect* dest,
    SDL_FRect dimensions
) {
    if (!length) {
        return;
    }
    char* copy = static_cast<char*>(malloc(length));
    std::memcpy(copy, chunk, length);
    // copy[length] = 0;
    std::vector<size_t> lines{0};
    for (size_t index = 0; index < length; index++) {
        if (chunk[index] == '\n') {
            copy[index] = 0;
            lines.push_back(index+1);
        }
    }
    for (size_t line : lines) {
        if (line == length) {
            break;
        }
        dest->w = 0;
        dest->h = 30;
        if (copy[line]) {
            size_t lineSize = strnlen(line+copy, length-line);
            size_t lineOffset = std::clamp((size_t)(dimensions.w/18), (size_t)0, lineSize);
            SDL_CHK(text(
                renderer,
                line + copy + lineOffset,
                &texture,
                font,
                &dest->w,
                &dest->h,
                lineSize - lineOffset,
                {0, 255, 0, 255}
            ));
            if (dest->y - dimensions.y >= dimensions.h) {
                dest->y -= dimensions.h;
                if (dest->x - dimensions.x >= dimensions.w) {
                    dest->x -= dimensions.w;
                    SDL_CHK(SDL_RenderTexture(renderer, texture, NULL, dest));
                    dest->x += dimensions.w;
                } else {
                    SDL_LogCritical(
                        CUSTOM_LOG_CATEGORY_TEXT,
                        "can't render when visStart.x > 0\n"
                    );
                }
                dest->y += dimensions.h;
            }
            SDL_DestroyTexture(texture);
            texture = NULL;
        }
        dest->x = dimensions.x;
        dest->y += dest->h;
    }
    if (copy[length-1]) {
        dest->y -= dest->h;
    } else {
        dest->w = 0;
    }
    free(copy);
}

size_t TextSection::coordsToIndex(int32_t window_x, int32_t window_y) {
    const size_t y = visStart.y + window_y;
    const size_t x = visStart.x + window_x;
    const size_t row = y/30;
    const size_t col = x/18;
    size_t currentRow = 0;
    size_t currentCol = 0;
    size_t index = 0;
    size_t i;
    for (i = 0; currentRow < row && i < fileSize; i++) {
        index = i + bufferSize*(i >= cursor);
        currentCol++;
        if (content[index] == '\n') {
            currentRow++;
            currentCol = 0;
        }
    }
    for (; currentCol < col && i < fileSize; i++) {
        index = i + bufferSize*(i >= cursor);
        currentCol++;
        if (content[index] == '\n') {
            break;
        }
    }
    return i;
}

void TextSection::drawFileText(SDL_Renderer* renderer, SDL_FRect dimensions, TTF_Font* font) const {
    SDL_Texture* texture = NULL;
    SDL_FRect dest = dimensions;
    drawTextChunk(
        content,
        cursor,
        renderer,
        texture,
        font,
        &dest,
        {dimensions.x, dimensions.y, visStart.x, visStart.y}
    );
    dest.x = dest.w + dimensions.x;
    drawTextChunk(
        content+cursor+bufferSize,
        fileSize-cursor,
        renderer,
        texture,
        font,
        &dest,
        {dimensions.x, dimensions.y, visStart.x, visStart.y}
    );
}

SDL_FRect TextSection::getCursorPos() const {
    SDL_FRect dest = SDL_FRect{0, 0, 2, 40};
    dest.y = -visStart.y;
    for (size_t c = 0; c < cursor; c++) {
        dest.x += 18;
        if (content[c] == '\n') {
            dest.y += 30;
            dest.x = -visStart.x;
        }
    }
    return dest;
}

void TextSection::drawCursor(SDL_Renderer* renderer, SDL_FRect dimensions) const {
    auto now = SDL_GetTicks();
    if (((now-lastCursorChange) / int(1000*0.5)) % 2) {
        return;
    }
    SDL_FRect dest = getCursorPos();
    dest.x += dimensions.x;
    dest.y += dimensions.y;
    SDL_CHK(SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255));
    SDL_CHK(SDL_RenderFillRect(renderer, &dest));
}

void TextSection::draw(SDL_Renderer* renderer, SDL_FRect dimensions, TTF_Font* font = defaultFont) const {
    dimensions = dimensions + SDL_FPoint{50, 50};
    dimensions.w -= 50;
    dimensions.h -= 50;
    width = dimensions.w;
    height = dimensions.h;
    drawCursor(renderer, dimensions);
    if (!fileSize) {
        return;
    }
    drawFileText(renderer, dimensions, font);
}

void TextSection::open(const char* file) {
    close();
    if (file) {
        fileHandle = fopen(file, "r+");
    } else {
        fileHandle = tmpfile();
    }
    assert(fileHandle);
    fseek(fileHandle, 0, SEEK_END);
    fileSize = ftell(fileHandle);
    rewind(fileHandle);
    free(content);
    content = static_cast<char*>(malloc(fileSize+1024));
    if (!content) {
        exit(-ENOMEM);
    }
    cursor = 0;
    bufferSize = 1024;
    // what about big files? do we lazy load lower lines as the user scrolls down?
    fread(content+cursor+bufferSize, fileSize, 1, fileHandle);
    if (file) {
        freopen(file, "w+", fileHandle);
        save();
    }
}

void TextSection::write(char c) {
    lastCursorChange = SDL_GetTicks();
    if (!bufferSize) {
        grow();
    }
    content[cursor] = c;
    bufferSize--;
    fileSize++;
    cursor++;
    const auto cursorPos = getCursorPos();
    if (cursorPos.y >= height) {
        visStart.y = cursorPos.y - height - 30;
    }
    if (cursorPos.y < 0) {
        visStart.y = cursorPos.y;
    }
}

void TextSection::del(movement to) {
    lastCursorChange = SDL_GetTicks();
    if (to & MOVEMENT_select) {
        to -= MOVEMENT_select;
    }
    switch(to) {
        case 0: // BACKSPACE -> one char back
            if (cursor == 0) {
                return;
            }
            cursor--;
            bufferSize++;
            fileSize--;
            break;
        case MOVEMENT_forward: // DEL -> one char forward
            if (cursor == fileSize) {
                return;
            }
            bufferSize++;
            fileSize--;
            break;
        case MOVEMENT_wordWise: { // CTRL+BACKSPACE -> one word back
            if (!cursor) {
                break;
            }
            char last = content[cursor-1];
            cursor--;
            bufferSize++;
            fileSize--;
            while (cursor && !isWordBreak(content[cursor-1], last)) {
                last = content[cursor-1];
                cursor--;
                bufferSize++;
                fileSize--;
            }
        } break;
        case MOVEMENT_wordWise+MOVEMENT_forward: { // CTRL+DEL -> one word forward
            // TODO: there is a bug in here. it's best you compare this code to the moveRel code that works
            if (cursor == fileSize) {
                break;
            }
            char last = content[cursor+bufferSize];
            bufferSize++;
            fileSize--;
            while (cursor < fileSize && !isWordBreak(content[cursor+bufferSize], last)) {
                last = content[cursor+bufferSize];
                bufferSize++;
                fileSize--;
            }
        } break;
        default:
            return;
    }
}

void TextSection::close() {
    if (!fileHandle) {
        return;
    }
    fclose(fileHandle);
    fileHandle = NULL;
}

void TextSection::save() const {
    assert(fileHandle);
    flush();
    fflush(fileHandle);
}

void TextSection::saveas(const char* newFile) {
    if (fileHandle) {
        close();
    }
    fileHandle = fopen(newFile, "w+");
    save();
}

void TextSection::moveRel(movement to) {
    lastCursorChange = SDL_GetTicks();
    switch(to) {
        case 0: // LEFT -> one char back
            if (cursor == 0) {
                break;
            }
            cursor--;
            content[cursor+bufferSize] = content[cursor];
            break;
        case MOVEMENT_forward: // RIGHT -> one char forward
            if (cursor == fileSize) {
                break;
            }
            content[cursor] = content[cursor+bufferSize];
            cursor++;
            break;
        case MOVEMENT_wordWise: // CTRL+LEFT -> one word back
            if (!cursor) {
                break;
            }
            cursor--;
            content[cursor+bufferSize] = content[cursor];
            while (cursor && !isWordBreak(content[cursor-1], content[cursor+bufferSize])) {
                cursor--;
                content[cursor+bufferSize] = content[cursor];
            }
            break;
        case MOVEMENT_wordWise+MOVEMENT_forward: // CTRL+RIGHT -> one word forward
            if (cursor == fileSize) {
                break;
            }
            content[cursor] = content[cursor+bufferSize];
            cursor++;
            while (cursor < fileSize && !isWordBreak(content[cursor+bufferSize], content[cursor-1])) {
                content[cursor] = content[cursor+bufferSize];
                cursor++;
            }
            break;
        case MOVEMENT_lineWise: // UP -> one line back
            {
                int32_t col = 0;
                int32_t lineSize = -1;
                bool end = false;
                bool foundUpperLine = false;
                while (cursor && !end) {
                    cursor--;
                    content[cursor+bufferSize] = content[cursor];
                    if (content[cursor+bufferSize] == '\n') {
                        end = foundUpperLine;
                        foundUpperLine = true;
                    }
                    if (foundUpperLine) {
                        lineSize++;
                    } else {
                        col++;
                    }
                }
                if (lineSize <= 0) {
                    break;
                }
                if (!foundUpperLine) {
                    break;
                }
                if (end) {
                    content[cursor] = content[cursor+bufferSize];
                    cursor++;
                }
                int32_t finalCol = std::min(col, lineSize);
                assert(finalCol >= 0);
                moveAbs(finalCol+cursor);
            }
            break;
        case MOVEMENT_lineWise + MOVEMENT_forward: // DOWN -> one line forward
            {
                int col = 0;
                for (size_t i = cursor-1; i != -1UL; i--) {
                    if (content[i] == '\n') {
                        break;
                    }
                    col++;
                }
                size_t end = cursor;
                while (end < fileSize && content[end+bufferSize] != '\n') {
                    end++;
                }
                size_t pos = end + (end < fileSize);
                for (
                    ;
                    pos < fileSize && content[pos+bufferSize] != '\n' && pos <= end+col;
                    pos++
                ) {}
                moveAbs(pos);
            }
            break;
        case MOVEMENT_full: // HOME -> beginning of line
            // TODO:
            // only go to end of whitespace, or if there is at least one character of and only whitespace up until cursor go to beginning of line
            while (cursor && content[cursor-1] != '\n') {
                cursor--;
                content[cursor+bufferSize] = content[cursor];
            }
            break;
        case MOVEMENT_full + MOVEMENT_forward: // END -> end of line
            while (cursor < fileSize && content[cursor] != '\n') {
                content[cursor] = content[cursor+bufferSize];
                cursor++;
            }
            break;
        case MOVEMENT_full + MOVEMENT_wordWise: // CTRL+HOME -> beginning of file
            std::memmove(content+bufferSize, content, cursor);
            cursor = 0;
            break;
        case MOVEMENT_full + MOVEMENT_wordWise + MOVEMENT_forward: // CTRL+END -> end of file
            std::memmove(content, content+cursor+bufferSize, fileSize-cursor);
            cursor = fileSize;
            break;
        default:
            return;
    }
    const auto cursorPos = getCursorPos();
    if (cursorPos.y >= height) {
        visStart.y = cursorPos.y - height - 30;
    }
    if (cursorPos.y < 0) {
        visStart.y = cursorPos.y;
    }
}

void TextSection::moveAbs(size_t to) {
    lastCursorChange = SDL_GetTicks();
    if (to > cursor) {
        // cursor moves right
        std::memmove(content+cursor, content+cursor+bufferSize, to-cursor);
    } else if (to < cursor) {
        // cursor moves left
        std::memmove(content+to+bufferSize, content+to, cursor-to);
    }
    cursor = to;
}

void TextSection::moveAbs(int32_t x, int32_t y) {
    moveAbs(coordsToIndex(x-50-EditorState::textOffset+9, y-50));
}

TextSection::~TextSection() {
    if (fileHandle) {
        flush();
        fclose(fileHandle);
        fileHandle = NULL;
    }
    free(content);
    content = nullptr;
}

TextSection::TextSection() {}

void TextSection::grow(size_t newSize) {
    const auto oldContent = content;
    content = static_cast<char*>(realloc(content, fileSize+newSize));
    if (!content) {
        content = oldContent;
        exit(-ENOMEM);
    }
    bufferSize = newSize;
    std::memmove(content+cursor+bufferSize, content+cursor, fileSize-cursor);
}

void TextSection::flush() const {
    assert(fileHandle);
    rewind(fileHandle);
    fwrite(content, cursor, 1, fileHandle);
    fwrite(content+cursor+bufferSize, fileSize-cursor, 1, fileHandle);
}
