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
#endif