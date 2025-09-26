#include "text.hpp"
#include <algorithm>
#include <cstdio>

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
    if (!file) {
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

static bool isWordBreak(char from, char to) {
    UNUSED(from);
    UNUSED(to);
    return true;
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
        nonAscii = ((buffer[cursor+gapSize-1] & 0xC0) == 0x80) && (buffer[cursor-1] & 0x80); // checking buffer[cursor-1] is redundant but better be safe than sorry for deleting an ascii character that preceeded a 0b10… char
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

void Text::up(std::vector<ssize_t>& newLines, size_t inLineOffset) {
    // TODO: use parameter inLineOffset

    const size_t startOfLineAbove = pos == newLines.begin() ? 0 : *(--pos) +1;
    
    
    const auto gapSize = bufferSize-fileSize;
    const auto newPos = startOfLineAbove+inLineOffset;
    std::memmove(
        buffer+newPos+gapSize,
        buffer+newPos,
        cursor-newPos);
    cursor = newPos;
}

void Text::down(std::vector<ssize_t>& newLines, size_t inLineOffset) {
    // TODO: use parameter inLineOffset
    const auto pos = std::lower_bound(newLines.begin(), newLines.end(), cursor);
    if (pos == newLines.end()) {
        ending();
        return;
    }
    const size_t startOfThisLine = pos == newLines.begin() ? 0 : *(pos-1) +1;
    size_t inLineOffset = 0;
    for (size_t byte = startOfThisLine; byte < cursor; byte++) {
        if (buffer[byte] & 0x80) {
            inLineOffset += static_cast<bool>(buffer[byte] & 0x40);
            // 11000011 counts as a character
            // 10110110 does not
            // together they are ö
        } else {
            inLineOffset++;
        }
    }
    const size_t startOfNextLine = *pos +1;
    const size_t endOfNextLine = (pos+1) == newLines.end() ? fileSize : *(pos+1);
    const size_t gapSize = bufferSize-fileSize;
    size_t newPos = startOfNextLine;
    for (size_t c = 0; c < inLineOffset && newPos < endOfNextLine; newPos++) {
        if (buffer[startOfNextLine+gapSize+c] & 0x80) {
            c += static_cast<bool>(buffer[startOfNextLine+gapSize+c] & 0x40);
        } else {
            c++;
        }
    }
    std::memmove(buffer+cursor, buffer+gapSize+cursor, newPos-cursor);
    cursor = newPos;
}

void Text::home(std::vector<ssize_t>& newLines) {
    const auto pos = std::lower_bound(newLines.begin(), newLines.end(), cursor);
    const size_t startOfThisLine = pos == newLines.begin() ? -1 : *(pos-1);
    const auto gapSize = bufferSize-fileSize;
    const auto newPos = startOfThisLine+1;
    std::memmove(
        buffer+newPos+gapSize,
        buffer+newPos,
        cursor-newPos);
    cursor = newPos;
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

std::pair<Text::Iterator, Text::Iterator> Text::getView(int startLine, int lineCount) const {  
    std::pair<Text::Iterator, Text::Iterator> ret;
    auto it = begin();
    int currentLine = 0;
    for (; it != end() && currentLine < startLine; ++it) {
        if (*it == '\n') {
            currentLine++;
        }
    }
    ret.first = it;
    for (; it != end() && currentLine < startLine+lineCount; ++it) {
        if (*it == '\n') {
            currentLine++;
        }
    }
    ret.second = it;
    return ret;
}

#endif