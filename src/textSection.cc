#include "editor.hpp"
#include <util.hpp>
#include <cstdio>

void TextSection::drawFileText(SDL_Renderer* renderer, SDL_FRect dimensions, TTF_Font* font) const {
    SDL_Texture* texture = NULL;
    SDL_FRect dest = dimensions;
    if (cursor) {
        SDL_CHK(text(
            renderer,
            content,
            &texture,
            font,
            &dest.w,
            &dest.h,
            cursor,
            {0, 255, 0, 255}
        ));
        SDL_CHK(SDL_RenderTexture(renderer, texture, NULL, &dest));
        SDL_DestroyTexture(texture);
        texture = NULL;
        dest.x += dest.w;
        dest.y += dest.h - 20;
    }
    if (fileSize-cursor) {
        SDL_CHK(text(
            renderer,
            content+cursor+bufferSize,
            &texture,
            font,
            &dest.w,
            &dest.h,
            fileSize-cursor,
            {0, 255, 0, 255}
        ));
        SDL_CHK(SDL_RenderTexture(renderer, texture, NULL, &dest));
        SDL_DestroyTexture(texture);
        texture = NULL;
        dest.x += dest.w;
        dest.y += dest.h - 20;
    }
}

void TextSection::drawCursor(SDL_Renderer* renderer, SDL_FRect dimensions) const {
    SDL_FRect dest = SDL_FRect{dimensions.x, dimensions.y, 2, 40};
    for (size_t c = 0; c < cursor; c++) {
        if (content[c] == '\n') {
            dest.y += 10;
            dest.x = dimensions.x;
        }
        dest.x += 18;
    }
    SDL_CHK(SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255));
    SDL_CHK(SDL_RenderFillRect(renderer, &dest));
}

void TextSection::draw(SDL_Renderer* renderer, SDL_FRect dimensions, TTF_Font* font = defaultFont) const {
    dimensions = dimensions + SDL_FPoint{50, 50};
    dimensions.w -= 50;
    dimensions.h -= 50;
    drawCursor(renderer, dimensions);
    if (!fileSize) {
        return;
    }
    drawFileText(renderer, dimensions, font);
}

void TextSection::open(const char* file) {
    if (file) {
        fileHandle = fopen(file, "w+");
    } else {
        fileHandle = tmpfile();
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
    assert(cursor == 0);
    fread(content+cursor+bufferSize, fileSize, 1, fileHandle);
}

void TextSection::write(char c) {
    if (!bufferSize) {
        grow();
    }
    content[cursor] = c;
    bufferSize--;
    fileSize++;
    cursor++;
}

void TextSection::del(movement to) {
    switch(to) {
        case 0: // LEFT -> one char back
            if (cursor == 0) {
                return;
            }
            cursor--;
            bufferSize++;
            fileSize--;
            break;
        case MOVEMENT_forward: // RIGHT -> one char forward
            if (cursor == fileSize) {
                return;
            }
            bufferSize++;
            fileSize--;
            break;
        case MOVEMENT_wordWise: // CTRL+LEFT -> one word back
            while (cursor && !isWordBreak(content[cursor-1], content[cursor])) {
                cursor--;
                bufferSize++;
                fileSize--;
            }
            break;
        case MOVEMENT_wordWise+MOVEMENT_forward: // CTRL+RIGHT -> one word forward
            while (cursor < fileSize && !isWordBreak(content[cursor+1], content[cursor])) {
                bufferSize++;
                fileSize--;
            }
            break;
        case MOVEMENT_full: // HOME -> beginning of line
            // TODO
            break;
        case MOVEMENT_full + MOVEMENT_forward: // END -> end of line
            // TODO
            break;
        case MOVEMENT_full + MOVEMENT_wordWise: // CTRL+HOME -> beginning of file
            // TODO
            break;
        case MOVEMENT_full + MOVEMENT_wordWise + MOVEMENT_forward: // CTRL+END -> end of file
            // TODO
            break;
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
        fclose(fileHandle);
    }
    fileHandle = fopen(newFile, "w+");
    save();
}

void TextSection::moveRel(movement to) {
    switch(to) {
        case 0: // LEFT -> one char back
            if (cursor == 0) {
                return;
            }
            cursor--;
            content[cursor+bufferSize] = content[cursor];
            break;
        case MOVEMENT_forward: // RIGHT -> one char forward
            if (cursor == fileSize) {
                return;
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
        case MOVEMENT_lineWise + MOVEMENT_forward: // DOWN -> one line forward
            // TODO
            break;
        case MOVEMENT_full: // HOME -> beginning of line
            // TODO
            break;
        case MOVEMENT_full + MOVEMENT_forward: // END -> end of line
            // TODO
            break;
        case MOVEMENT_full + MOVEMENT_wordWise: // CTRL+HOME -> beginning of file
            // TODO
            break;
        case MOVEMENT_full + MOVEMENT_wordWise + MOVEMENT_forward: // CTRL+END -> end of file
            // TODO
            break;
        default:
            return;
    }
}

void TextSection::moveAbs(size_t to) {
    if (to > cursor) {
        // cursor moves right
        std::memmove(content+cursor, content+cursor+bufferSize, to-cursor);
    } else if (to < cursor) {
        // cursor moves left
        std::memmove(content+to+bufferSize, content+to, cursor-to);
    }
    cursor = to;
}

TextSection::~TextSection() {
    if (fileHandle) {
        flush();
        fclose(fileHandle);
        fileHandle = NULL;
    }
    if (content) {
        free(content);
        content = nullptr;
    }
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
}

void TextSection::flush() const {
    assert(fileHandle);
    rewind(fileHandle);
    fwrite(content, cursor, 1, fileHandle);
    fwrite(content+cursor+bufferSize, fileSize-cursor, 1, fileHandle);
}
