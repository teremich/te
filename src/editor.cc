#include "editor.hpp"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_mouse.h"
#include "logging.hpp"
#include "util.hpp"
#include <algorithm>

#define S64SIGN_BIT (~(static_cast<size_t>(-1) >> 1))

SDL_FPoint drawLine(const char* buffer, int len, const SDL_FRect& into, TTF_Font* font, SDL_Renderer* renderer) {
    if (!len) {
        return {0, (float)TTF_GetFontHeight(font)};
    }
    if (len == -1) {
        len = 0;
    }
    SDL_Surface* renderedStrip = TTF_RenderText_Blended(font, buffer, len, SDL_Color{255, 255, 255, 255});
    SDL_CHK(!!renderedStrip);
    // SDL_CHK(SDL_GetSurfaceClipRect(renderedStrip, &srcrect));
    // if (srcrect.w > into.w) {
    //     srcrect.w = into.w;
    // }
    // if (srcrect.h > into.h) {
    //     srcrect.h = into.h;
    // }
    // SDL_CHK(SDL_SetSurfaceClipRect(renderedStrip, &srcrect));
    auto stripTexture = SDL_CreateTextureFromSurface(renderer, renderedStrip);
    SDL_CHK(!!stripTexture);
    float width, height;
    SDL_CHK(SDL_GetTextureSize(stripTexture, &width, &height));
    SDL_FRect srcrect = {0, 0, width, height};
    SDL_FRect dstrect{into.x, into.y, srcrect.w, srcrect.h};
    SDL_CHK(SDL_RenderTexture(renderer, stripTexture, &srcrect, &dstrect));
    SDL_DestroySurface(renderedStrip);
    SDL_DestroyTexture(stripTexture);
    return {srcrect.w, srcrect.h};
}

void drawCursor(const SDL_FRect& into, SDL_Renderer* renderer, TTF_Font* font) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_FRect cursor = {into.x, into.y, 2, static_cast<float>(TTF_GetFontHeight(font))};
    SDL_RenderFillRect(renderer, &cursor);
}

static void renderText(SDL_Renderer* renderer, TTF_Font* font, SDL_FRect& into, Text& text, ssize_t startLine, const std::vector<ssize_t>& newLines) {
    static char buffer[16];
    assert(startLine >= 0);
    char lineNumber[5]{};
    int lineNumberSize = SDL_snprintf(lineNumber, 5, "%zu", (startLine+1) % 100'000);
    std::memmove(lineNumber+5-lineNumberSize, lineNumber, lineNumberSize);
    std::memset(lineNumber, 0, 5-lineNumberSize);
    auto it = text.begin()+(startLine > 0 ? newLines[startLine-1]+1 : 0);
    // const ssize_t startOfThisLine = (it == text.begin()) ? 0 : *it +1;
    size_t drawnChars = 0;
    int i = 0;
    SDL_FPoint orig{into.x, into.w};
    const auto lineNumberWidth = 5*20+10;
    drawLine(lineNumber+5-lineNumberSize, lineNumberSize, into, font, renderer);
    into.x += lineNumberWidth;
    into.w -= lineNumberWidth;
    while (into.h > 0) {
        if (it.cursorPos == it.pos) {
            SDL_FPoint drawn = drawLine(buffer, i, into, font, renderer);
            into.x += drawn.x;
            into.w -= drawn.x;
            i = 0;
            drawCursor(into, renderer, font);
        }
        if (it == text.end()) {
            drawLine(buffer, i, into, font, renderer);
            return;
        }
        if (*it == '\n') {
            SDL_FPoint drawn = drawLine(buffer, i, into, font, renderer);
            ++it;
            into.y += drawn.y;
            into.h -= drawn.y;
            into.x = orig.x;
            into.w = orig.y;
            i = 0;
            drawnChars = 0;
            for (int j = 4; j >= 0; j--) {
                lineNumber[j]++;
                if (lineNumber[j] == 1) {
                    lineNumber[j] = '1';
                    lineNumberSize++;
                    assert(static_cast<size_t>(lineNumberSize) <= SDL_arraysize(lineNumber));
                    break;
                }
                if (lineNumber[j] > '9') {
                    lineNumber[j] = '0';
                } else {
                    break;
                }
            }
            drawLine(lineNumber+5-lineNumberSize, lineNumberSize, into, font, renderer);
            // printf(
            //     "lineNumber: %02x %02x %02x %02x %02x\n"
            //     "lineNumberSize: %d\n",
            //     lineNumber[0],
            //     lineNumber[1],
            //     lineNumber[2],
            //     lineNumber[3],
            //     lineNumber[4],
            //     lineNumberSize
            // );
            into.x += lineNumberWidth;
            into.w -= lineNumberWidth;
            continue;
        }
        if (*it == '\t') {
            SDL_FPoint drawn = drawLine(buffer, i, into, font, renderer);
            into.x += drawn.x;
            into.w -= drawn.x;
            i = 0;
            *(uint32_t*)buffer = 0x01010101 * ' '; // 4 spaces
            // contact me if you know how to convert 0b100 to 0x01010101 efficiently
            drawn = drawLine(buffer, 4-(drawnChars%4), into, font, renderer);
            drawnChars += 4-(drawnChars%4);
            into.x += drawn.x;
            into.w -= drawn.x;
            ++it;
            continue;
        }
        bool closeToEndAndIteratorDoesNotPointIntoAUTF8Byte = i > static_cast<int>(SDL_arraysize(buffer)-4) && ((*it & 0xC0) != 0x80);
        if (closeToEndAndIteratorDoesNotPointIntoAUTF8Byte || i == SDL_arraysize(buffer)) {
            SDL_FPoint drawn = drawLine(buffer, i, into, font, renderer);
            into.x += drawn.x;
            into.w -= drawn.x;
            i = 0;
        }
        buffer[i++] = *it;
        if (*it & 0x80) {
            drawnChars += static_cast<bool>(*it & 0x40);
        } else {
            drawnChars++;
        }
        ++it;
    }
}

void Editor::render(SDL_Renderer* renderer, SDL_FRect into, TTF_Font* font) const {
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderFillRect(renderer, &into);
    SDL_FRect canvas{into.x+30, into.y+10, into.w-50, into.h-20};
    if (currentFile.index < files.size) {
        const char* filename = filenames[currentFile.index].c_str();
        if (!*filename) {
            filename = "Untitled";
        }
        const auto drawn = drawLine(filename, -1, canvas, font, renderer);
        // MOVE DOWN BY THE DRAWN AMOUNT
        canvas.y += drawn.y;
        canvas.h -= drawn.y;
        // MOVE DOWN ADDITIONAL 50 PX
        canvas.y += 50;
        canvas.h -= 50;
        // MOVE RIGHT 20 PX
        canvas.x += 20;
        canvas.w -= 20;
        ssize_t maxLines = canvas.h / TTF_GetFontHeight(font) - 1;
        if (currentFile.startLine < 0) {
            currentFile.startLine = currentFile.startLine ^ S64SIGN_BIT;
            if (currentFile.startLine > currentFile.numLinesBeforeCursor) {
                // cursor in above startLine and startLine was invalidated
                currentFile.startLine = currentFile.numLinesBeforeCursor;
            }
            if (maxLines+currentFile.startLine < currentFile.numLinesBeforeCursor) {
                // cursor is below startLine+maxLines and startLine was invalidated
                currentFile.startLine = currentFile.numLinesBeforeCursor-maxLines;
            }
        }
        if (currentFile.startLine > static_cast<ssize_t>(currentFile.newLineIndices.size())) {
            // startLine larger than file allows
            currentFile.startLine = currentFile.newLineIndices.size();
        }
        renderText(renderer, font, canvas, files.items[currentFile.index], currentFile.startLine, currentFile.newLineIndices);
    }
}

void Editor::update() {
    if (currentFile.index >= files.size) {
        return;
    }
    currentFile.newLineIndices.clear();
    const auto end = std::end(files.items[currentFile.index]);
    currentFile.numLinesBeforeCursor = -1;
    for (auto it = std::begin(files.items[currentFile.index]); it != end; ++it) {
        if (it.pos == it.cursorPos) {
            currentFile.numLinesBeforeCursor = currentFile.newLineIndices.size();
        }
        if (*it == '\n') {
            currentFile.newLineIndices.push_back(it.pos);
        }
    }
    if (currentFile.numLinesBeforeCursor < 0 && end.pos == end.cursorPos) {
        currentFile.numLinesBeforeCursor = currentFile.newLineIndices.size();
    }
}

void Editor::write(const char* str) {
    if (currentFile.index >= files.size) {
        return;
    }
    files.items[currentFile.index].insert(str);
    currentFile.startLine |= S64SIGN_BIT;
}


static void SDLCALL saveFileCallback(void *userdata, const char * const *filelist, int filter) {
    UNUSED(filter);
    if (!filelist) {
        SDL_LogWarn(CUSTOM_LOG_CATEGORY_EDITOR, "Error while saving file: %s\n", SDL_GetError());
        return;
    }
    if (!filelist[0]) {
        // cancelled the dialog
        return;
    }
    if (!*filelist[0]) {
        // empty filename
        return;
    }
    Editor* editor = reinterpret_cast<Editor*>(userdata);
    editor->saveAs(filelist[0]);
}

static void SDLCALL openFileCallback(void* userdata, const char * const *filelist, int filter) {
    UNUSED(filter);
    if (!filelist) {
        SDL_LogWarn(CUSTOM_LOG_CATEGORY_EDITOR, "Error while opening file(s): %s\n", SDL_GetError());
        return;
    }
    for (const char* const* file = filelist; *file; file++) {
        const char* filename = *file;
        if (!*filename) {
            // filename in filelist has 0 size
            continue;
        }
        Editor* editor = reinterpret_cast<Editor*>(userdata);
        editor->open(filename);
    }
}

static_assert(std::is_same<decltype(&openFileCallback), SDL_DialogFileCallback>::value);

void Editor::close(size_t index) {
    if (index >= files.size) {
        return;
    }
    files.items[index] = files.pop();
    filenames.at(index) = *filenames.rbegin();
    filenames.pop_back();
}

void Editor::updateInlineOffset() {
    if (currentFile.index >= files.size) {
        return;
    }
    update();
    const auto cursor = files.items[currentFile.index].begin().cursorPos;
    const auto endOfThisLine = std::lower_bound(currentFile.newLineIndices.begin(), currentFile.newLineIndices.end(), cursor);
    const auto startOfThisLine = endOfThisLine == currentFile.newLineIndices.begin() ? 0 : *(endOfThisLine-1)+1;
    currentFile.inlineOffset = 0;
    for (size_t c = startOfThisLine; c < cursor; c++) {
        const char currentChar = *(files.items[currentFile.index].begin()+c);
        if (currentChar & 0x80) {
            currentFile.inlineOffset += static_cast<bool>(currentChar & 0x40);
        } else if (currentChar == '\t') {
            currentFile.inlineOffset += 4;
        } else {
            currentFile.inlineOffset++;
        }
    }
}

void Editor::scroll(SDL_MouseWheelEvent wheel) const {
    if (currentFile.startLine < 0) {
        return;
    }
    currentFile.startLine -= wheel.integer_y * (1-2*wheel.direction);
    if (currentFile.startLine < 0) {
        currentFile.startLine = 0;
    }
}

void Editor::write(SDL_KeyboardEvent key) {
    SDL_Event e{
        .key = key
    };
    const bool lctrl = key.mod & SDL_KMOD_LCTRL;
    if (key.key == SDLK_O && lctrl) {
        // LCTRL + O
        SDL_ShowOpenFileDialog(openFileCallback, this, SDL_GetWindowFromEvent(&e), NULL, 0, folder, true);
        return;
    }
    if (key.key == SDLK_N && lctrl) {
        // LCTRL + N
        currentFile.index = files.push(Text());
        filenames.push_back({});
        return;
    }
    if (currentFile.index >= files.size) {
        return;
    }
    const bool ctrl = key.mod & SDL_KMOD_CTRL;
    switch(key.scancode) {
        case SDL_SCANCODE_DELETE:
            files.items[currentFile.index].del(ctrl);
            return;
        case SDL_SCANCODE_BACKSPACE:
            files.items[currentFile.index].backspace(ctrl);
            currentFile.inlineOffset--;
            return;
        case SDL_SCANCODE_RETURN:
            files.items[currentFile.index].insert('\n');
            // TODO: add whitespace from current line and add whitespace to text behind cursor
            currentFile.inlineOffset = 0;
            currentFile.startLine |= S64SIGN_BIT;
            return;
        case SDL_SCANCODE_UP:
            files.items[currentFile.index].up(currentFile.newLineIndices, currentFile.inlineOffset);
            currentFile.startLine |= S64SIGN_BIT;
            return;
        case SDL_SCANCODE_DOWN:
            files.items[currentFile.index].down(currentFile.newLineIndices, currentFile.inlineOffset);
            currentFile.startLine |= S64SIGN_BIT;
            return;
        case SDL_SCANCODE_LEFT:
            files.items[currentFile.index].left(ctrl);
            currentFile.startLine |= S64SIGN_BIT;
            updateInlineOffset();
            return;
        case SDL_SCANCODE_RIGHT:
            files.items[currentFile.index].right(ctrl);
            currentFile.startLine |= S64SIGN_BIT;
            updateInlineOffset();
            return;
        case SDL_SCANCODE_HOME:
            if (ctrl) {
                files.items[currentFile.index].beginning();
                currentFile.inlineOffset = 0;
            } else {
                currentFile.inlineOffset = files.items[currentFile.index].home(currentFile.newLineIndices);
            }
            currentFile.startLine |= S64SIGN_BIT;
            return;
        case SDL_SCANCODE_END:
            ctrl ? files.items[currentFile.index].ending() : files.items[currentFile.index].ende(currentFile.newLineIndices);
            updateInlineOffset();
            currentFile.startLine |= S64SIGN_BIT;
            return;
        case SDL_SCANCODE_TAB:
            static constexpr SDL_Keymod KMOD_TOGGLE_KEYS = SDL_KMOD_CAPS | SDL_KMOD_NUM | SDL_KMOD_SCROLL;
            if (!(key.mod & ~KMOD_TOGGLE_KEYS)) {
                files.items[currentFile.index].insert('\t');
                currentFile.startLine |= S64SIGN_BIT;
                return;
            } break;
        default:
            // SDL_LogCritical(CUSTOM_LOG_CATEGORY_INPUT, "didn't handle special key %u\n", key);
            break;
    }
    if (key.key == SDLK_S && lctrl) {
        if (key.mod & SDL_KMOD_SHIFT) {
            // LCTRL + SHIFT + S
            SDL_ShowSaveFileDialog(saveFileCallback, this, SDL_GetWindowFromEvent(&e), NULL, 0, folder);
            return;
        }
        // LCTRL + S
        files.items[currentFile.index].save(filenames[currentFile.index].c_str());
        return;
    }
    if (key.key == SDLK_W && lctrl) {
        close(currentFile.index);
        if (currentFile.index >= files.size) {
            currentFile.index = files.size-1;
        }
        return;
    }
    if (key.key == SDLK_TAB && lctrl) {
        if (!files.size) {
            return;
        }
        if (key.mod & SDL_KMOD_SHIFT) {
            // CTRL + SHIFT + TAB
            switchTo((currentFile.index - 1 + files.size) % files.size);
            return;
        }
        // CTRL + TAB
        switchTo((currentFile.index + 1) % files.size);
        return;
    }
    // TODO: Copy, Cut, Paste
}

void Editor::moveTo(SDL_MouseButtonEvent button) {
    if (button.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        return;
    }
    if (button.button == SDL_BUTTON_LEFT) {
        switch((button.clicks-1) % 3) {
            case 0:
                // TODO: add clicking into the text field
                // [ ] calculate X, Y of mouse button
                // [ ] calculate line, column from Y and X
                // [ ] add startLine to line
                // [ ] find new position by looking up line in currentFile.newLineIndices
                // [ ] move the cursor to the new position
                break;
            case 1:
                // TODO: select word
                break;
            case 2:
                // TODO: select line
                break;
        }
    }
}

void Editor::switchTo(size_t index) {
    if (index >= files.size) {
        return;
    }
    currentFile = {
        index,
        0,
        {},
        0,
        -1
    };
    updateInlineOffset();
}

size_t Editor::open(const char* relativeFilePath) {
    filenames.push_back(relativeFilePath);
    currentFile.index = files.push(Text(relativeFilePath));
    // currentFile.inlineOffset = -1;
    currentFile.startLine = S64SIGN_BIT;
    updateInlineOffset();
    assert(currentFile.index == files.size-1);
    assert(currentFile.index == filenames.size()-1);
    return currentFile.index;
}

Editor::~Editor() = default;
