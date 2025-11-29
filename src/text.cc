#include "text.hpp"
#include <algorithm>
#include <cstdio>
#include <options.hpp>

#if ROPE

void Rope::print() {
    if (length < 0) {
        printf("{\n\"length\": %ld, \"left\": ", -length);
        children.left->print();
        printf(", \"right\": ");
        children.right->print();
        printf("}\n");
    } else {
        printf("{\n\"length\": %ld, \"size\": %zu,\n", length, string.size);
        printf("\"content\": \"");
        for (ssize_t i = 0; i < length; i++) {
            printf("%c", string.content[i]);
        }
        printf("\"\n}\n");
    }
}

void Text::print() {
    printf("{\"cursor\": %lu, \"rope\":\n", cursor);
    rope.print();
    printf("}\n");
}
void Text::insert(char c) {
    rope.insert(c, cursor);
    cursor++;
}
void Text::insert(const char* string) {
    for (const char* c = string; *c; c++) {
        insert(*c);
    }
}
// void moveRel();
Rope::Iterator Text::getView(int startLine, int lineCount) const {
    UNUSED(startLine);
    UNUSED(lineCount);
    return begin();
}
#else

const char* untitled = "Untitled";

Text::Text() {
    bufferSize = 1024;
    buffer = (char*)malloc(bufferSize);
    fileSize = 0;
    cursor = 0;
}

Text::Text(const char* file) : cursor(0) {
    FILE* f = fopen(file, "r");
    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    bufferSize = fileSize + 1024;
    buffer = (char*) malloc(bufferSize);
    assert(fread(buffer+bufferSize-fileSize, fileSize, 1, f) == 1);
    fclose(f);
}

Text& Text::operator=(Text&& moveFrom) {
    this->~Text();
    buffer = moveFrom.buffer;
    bufferSize = moveFrom.bufferSize;
    fileSize = moveFrom.fileSize;
    cursor = moveFrom.cursor;
    moveFrom.buffer = nullptr;
    moveFrom.cursor = 0;
    moveFrom.fileSize = 0;
    moveFrom.bufferSize = 0;
    return *this;
}

Text::Text(Text&& moveFrom) :
    buffer(moveFrom.buffer),
    bufferSize(moveFrom.bufferSize),
    cursor(moveFrom.cursor),
    fileSize(moveFrom.fileSize) {
    moveFrom.fileSize = 0;
    moveFrom.bufferSize = 0;
    moveFrom.buffer = nullptr;
    moveFrom.cursor = 0;
}

Text::~Text() {
    if (buffer) {
        free(buffer);
    }
}

void Text::load(const char* file) {
    if (buffer) {
        free(buffer);
        buffer = NULL;    
    }
    FILE* f = fopen(file, "r");
    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    bufferSize = fileSize + 1024;
    buffer = (char*) malloc(bufferSize);
    assert(fread(buffer, fileSize, 1, f) == 1);
    cursor = 0;
    fclose(f);
}

void Text::save(const char* file) const {
    if (!file || !*file) {
        return;
    }
    FILE* f = fopen(file, "w+");
    assert(f);
    fwrite(buffer, cursor, 1, f);
    fwrite(buffer+bufferSize-fileSize+cursor, fileSize-cursor, 1, f);
    fclose(f);
}

void Text::insert(char c) {
    if (bufferSize == fileSize) {
        bufferSize += 1024;
        const size_t gapSize = bufferSize-fileSize;
        buffer = (char*) realloc(buffer, bufferSize);
        std::memmove(buffer+cursor+gapSize, buffer+cursor, fileSize-cursor);
    }
    buffer[cursor++] = c;
    fileSize++;
}

void Text::insert(const char* str) {
    const auto len = strlen(str);
    while (bufferSize-fileSize < len) {
        bufferSize += 1024;
        const size_t gapSize = bufferSize-fileSize;
        buffer = (char*) realloc(buffer, bufferSize);
        std::memmove(buffer+cursor+gapSize, buffer+cursor, fileSize-cursor);
    }
    strncpy(buffer+cursor, str, len);
    cursor += len;
    fileSize += len;
}

static constexpr bool isWordBreak(char from, char to) {
    const bool fromIsUpper = 'A' <= from && from <= 'Z';
    const bool fromIsLower = 'a' <= from && from <= 'z';
    const bool fromIsNumber = '0' <= from && from <= '9';
    const bool fromIsNotSpecial = from == '_' and options.underscore_is_word_break;
    const bool fromIsAlphaNum = fromIsUpper || fromIsLower || fromIsNumber || fromIsNotSpecial;
    const bool toIsUpper = 'A' <= to && to <= 'Z';
    const bool toIsLower = 'a' <= to && to <= 'z';
    const bool toIsNumber = '0' <= to && to <= '9';
    const bool toIsNotSpecial = to == '_' and options.underscore_is_word_break;
    const bool toIsAlphaNum = toIsUpper || toIsLower || toIsNumber || toIsNotSpecial;
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
    
    
    //                     FROM IS ALPHA NUM | FROM IS WHITE SPACE | FROM IS SPECIAL CHAR
    // TO IS ALPHA NUM          false                   false               true
    // TO IS WHITE SPACE        true                    false               true
    // TO IS SPECIAL CHAR       true                    false               false
    return !fromIsWhiteSpace && (fromIsSpecial != toIsSpecial || fromIsAlphaNum != toIsAlphaNum);
}

size_t Text::getFileSize() const {
    return fileSize;
}

void Text::moveTo(ssize_t newPos) {
    if (newPos < 0) {
        return;
    }
    if (static_cast<size_t>(newPos) < cursor) {
        if (newPos) {
            if ((buffer[newPos-1] & 0x80) && (buffer[newPos] & 0xC0) == 0x80) {
                moveTo(newPos+1);
                return;
            }
        }
        std::memmove(buffer+bufferSize-fileSize+newPos, buffer+newPos, cursor-newPos);
    } else {
        if (
            newPos &&
            (buffer[newPos-1 + (bufferSize-fileSize)*(static_cast<size_t>(newPos) > cursor)] & 0x80)
            && (buffer[newPos + (bufferSize-fileSize)] & 0xC0) == 0x80
        ) {
            moveTo(newPos+1);
            return;
        }
        std::memmove(buffer+cursor, buffer+cursor+bufferSize-fileSize, newPos-cursor);
    }
    cursor = newPos;
}

void Text::backspace(bool wordWise) {
    if (!cursor) {
        return;
    }
    bool needMoreForWholeWord, nonAscii;
    do {
        cursor--;
        fileSize--;
        if (!cursor) {
            return;
        }
        const auto gapSize = bufferSize-fileSize;
        needMoreForWholeWord = wordWise && !isWordBreak(buffer[cursor+gapSize-1], buffer[cursor-1]);
        nonAscii = ((buffer[cursor] & 0xC0) == 0x80) && (buffer[cursor-1] & 0x80); // checking buffer[cursor-1] is redundant but better be safe than sorry for deleting an ascii character that preceeded a 0b10… char
    } while (nonAscii || needMoreForWholeWord);
}

void Text::left(bool wordWise) {
    if (!cursor) {
        return;
    }
    const auto gapSize = bufferSize-fileSize;
    bool needMoreForWholeWord, nonAscii;
    do {
        --cursor;
        buffer[cursor+gapSize] = buffer[cursor];
        if (!cursor) {
            return;
        }
        needMoreForWholeWord = wordWise && !isWordBreak(buffer[cursor], buffer[cursor-1]);
        nonAscii = buffer[cursor-1] & 0x80 && ((buffer[cursor] & 0xC0)== 0x80); // checking buffer[cursor-1] is redundant but helps if the file is not utf8
    } while(nonAscii || needMoreForWholeWord);
}

void Text::right(bool wordWise) {
    if (cursor == fileSize) {
        return;
    }
    const auto gapSize = bufferSize-fileSize;
    bool needMoreForWholeWord, nonAscii;
    do {
        buffer[cursor] = buffer[cursor+gapSize];
        ++cursor;
        if (cursor == fileSize) {
            return;
        }
        needMoreForWholeWord = wordWise && !isWordBreak(buffer[cursor-1], buffer[cursor+gapSize]);
        nonAscii = ((buffer[cursor+gapSize] & 0xC0) == 0x80) && (buffer[cursor-1] & 0x80); // checking buffer[cusor-1] is redundant
    } while (nonAscii || needMoreForWholeWord);
}

void Text::ending() {
    std::memmove(buffer+cursor, buffer+cursor+bufferSize-fileSize, fileSize-cursor);
    cursor = fileSize;
}

void Text::beginning() {
    std::memmove(buffer+bufferSize-fileSize, buffer, cursor);
    cursor = 0;
}

void Text::del(bool wordWise) {
    if (cursor == fileSize) {
        return;
    }
    bool needMoreForWholeWord, nonAscii;
    do {
        --fileSize;
        if (cursor == fileSize) {
            return;
        }
        const auto gapSize = bufferSize-fileSize;
        needMoreForWholeWord = wordWise && !isWordBreak(buffer[cursor+gapSize], buffer[cursor-1]);
        nonAscii = (buffer[cursor+gapSize-1] & 0x80) && ((buffer[cursor+gapSize] & 0xC0) == 0x80);
    } while (nonAscii || needMoreForWholeWord);
}

void Text::up(std::vector<ssize_t>& newLines, ssize_t inLineOffset) {
    const auto endOfThisLine = std::lower_bound(newLines.begin(), newLines.end(), cursor);
    if (endOfThisLine == newLines.begin()) {
        return beginning();
    }
    const auto startOfThisLine = endOfThisLine-1;
    if (inLineOffset < 0) {
        // inLineOffset = cursor-*(startOfThisLine)+1;
        for (size_t c = *(startOfThisLine)+1; c < cursor; c++) {
            if (buffer[c] & 0x80) {
                inLineOffset += static_cast<bool>(buffer[c] & 0x40);
            } else if (buffer[c] == '\t') {
                inLineOffset += 4;
            } else {
                inLineOffset++;
            }
        }
    }
    const size_t startOfLineAbove = startOfThisLine == newLines.begin() ? 0 : *(startOfThisLine-1) +1;
    const auto gapSize = bufferSize-fileSize;
    auto newPos = startOfLineAbove;
    for (ssize_t c = 0; c < inLineOffset && static_cast<ssize_t>(newPos) < *startOfThisLine; newPos++) {
        if (buffer[newPos+gapSize] & 0x80) {
            c += static_cast<bool>(buffer[newPos+gapSize] & 0x40);
        } else if (buffer[newPos+gapSize] == '\t') {
            c += 4;
        } else {
            c++;
        }
    }
    std::memmove(
        buffer+newPos+gapSize,
        buffer+newPos,
        cursor-newPos);
    cursor = newPos;
}

void Text::down(std::vector<ssize_t>& newLines, ssize_t inLineOffset) {
    const auto endOfThisLine = std::lower_bound(newLines.begin(), newLines.end(), cursor);
    if (endOfThisLine == newLines.end()) {
        return ending();
    }
    const auto startOfThisLine = endOfThisLine == newLines.begin() ? 0 : *(endOfThisLine-1) +1;
    if (inLineOffset < 0) {
        // inLineOffset = cursor-*(startOfThisLine)+1;
        for (size_t c = startOfThisLine; c < cursor; c++) {
            if (buffer[c] & 0x80) {
                inLineOffset += static_cast<bool>(buffer[c] & 0x40);
            } else if (buffer[c] == '\t') {
                inLineOffset += 4;
            } else {
                inLineOffset++;
            }
        }
    }
    const auto startOfNextLinePosition = *endOfThisLine +1;
    const std::vector<ssize_t>::iterator endOfNextLine = endOfThisLine+1;
    const ssize_t endOfNextLinePosition = endOfNextLine == newLines.end() ? fileSize : *endOfNextLine;
    const auto gapSize = bufferSize-fileSize;
    ssize_t newPos = startOfNextLinePosition;
    for (ssize_t c = 0; c < inLineOffset && newPos < endOfNextLinePosition; newPos++) {
        if (buffer[newPos+gapSize] & 0x80) {
            c += static_cast<bool>(buffer[newPos+gapSize] & 0x40);
        } else if (buffer[newPos+gapSize] == '\t') {
            c += 4;
        } else {
            c++;
        }
    }
    std::memmove(buffer+cursor, buffer+gapSize+cursor, newPos-cursor);
    cursor = newPos;
}

ssize_t Text::home(std::vector<ssize_t>& newLines) {
    const auto endOfThisLine = std::lower_bound(newLines.begin(), newLines.end(), cursor);
    const auto gapSize = bufferSize-fileSize;
    const size_t startOfThisLine = endOfThisLine == newLines.begin() ? 0 : *(endOfThisLine-1)+1;
    size_t endOfWhiteSpace;
    for (endOfWhiteSpace = startOfThisLine; endOfWhiteSpace < static_cast<size_t>(*endOfThisLine); endOfWhiteSpace++) {
        if (not isWhiteSpace(buffer[endOfWhiteSpace])) {
            break;
        }
    }

    if (cursor == startOfThisLine && endOfWhiteSpace > cursor) {
        // cursor moves right
        std::memmove(buffer+cursor, buffer+cursor+gapSize, endOfWhiteSpace-cursor);
        cursor = endOfWhiteSpace;
        return endOfWhiteSpace-startOfThisLine;
    }
    size_t newPos = endOfWhiteSpace < cursor ? endOfWhiteSpace : startOfThisLine;
    // cursor moves left
    std::memmove(
        buffer+newPos+gapSize,
        buffer+newPos,
        cursor-newPos);
    cursor = newPos;
    return cursor - startOfThisLine;
}

void Text::ende(std::vector<ssize_t>& newLines) {
    const auto pos = std::lower_bound(newLines.begin(), newLines.end(), cursor);
    if (pos == newLines.end()) {
        ending();
        return;
    }
    const size_t startOfNextLine = *pos;
    const size_t newPos = startOfNextLine;
    std::memmove(buffer+cursor, buffer+bufferSize-fileSize+cursor, newPos-cursor);
    cursor = newPos;
}

void Text::print() const {
    for (size_t i = 0; i < cursor; i++) {
        printf("%c", buffer[i]);
    }
    for (size_t i = 0; i < fileSize-cursor; i++) {
        printf("%c", (buffer+bufferSize-fileSize+cursor)[i]);
    }
}

#endif