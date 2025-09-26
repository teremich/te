#include "text.hpp"
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

Text::Text() {
    bufferSize = 1024;
    filename = NULL;
    buffer = (char*)malloc(bufferSize);
    fileSize = 0;
    cursor = 0;
}

Text::Text(const char* file) : filename(file), cursor(0) {
    FILE* f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    bufferSize = fileSize + 1024;
    buffer = (char*) malloc(bufferSize);
    assert(fread(buffer, fileSize, 1, f) == 1);
    fclose(f);
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
    filename = file;
    FILE* f = fopen(filename, "r");
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
        file = filename;
    }
    if (!file) {
        return;
    }
    FILE* f = fopen(file, "w+");
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

void Text::backspace() {
    if (!cursor) {
        return;
    }
    cursor--;
    fileSize--;
}

void Text::left(bool wordWise) {
    UNUSED(wordWise);
    if (!cursor) {
        return;
    }
    --cursor;
    buffer[cursor+bufferSize-fileSize] = buffer[cursor];
}

void Text::right(bool wordWise) {
    UNUSED(wordWise);
    if (cursor == fileSize) {
        return;
    }
    buffer[cursor] = buffer[cursor+bufferSize-fileSize];
    ++cursor;
}

void Text::ending() {
    std::memmove(buffer+cursor, buffer+cursor+bufferSize-fileSize, fileSize-cursor);
    cursor = fileSize;
}

void Text::beginning() {
    std::memmove(buffer+bufferSize-fileSize, buffer, cursor);
    cursor = 0;
}

void Text::del() {
    if (cursor > --fileSize) {
        cursor = fileSize;
    }
}

void Text::up() {
    assert(false && "TODO!");
}

void Text::down() {
    assert(false && "TODO!");
}

void Text::home() {
    assert(false && "TODO!");
}

void Text::ende() {
    assert(false && "TODO!");
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