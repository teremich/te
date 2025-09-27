#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <util.hpp>
#include <cassert>

#define ROPE 0

#if ROPE
#include <algorithm>
#include <iterator>

struct Rope{
    // positive or zero means it is a leaf
    // negative means its absolute value is the sum of all sizes of the leafs to the left of it
    ssize_t length = 0;
    static const ssize_t maxSize = 1024;
    union{
        struct {
            Rope *left, *right;
        } children;
        Array string = {nullptr, 0};
    };
    class Iterator{
        Iterator* child = nullptr;
        const Rope* parent = nullptr;
        size_t pos = 0;
        public:
        Iterator() = default;
        Iterator(const Rope* self, size_t pos)
         : parent(self), pos(pos)
        {
            if (parent->length < 0) {
                if ((ssize_t)pos < -parent->length) {
                    child = new Iterator(parent->children.left, pos);
                } else {
                    child = new Iterator(parent->children.right, pos+parent->length);
                }
            } else {
                assert((ssize_t)pos <= parent->length);
            }
        }
        Iterator& operator++() {
            if (child) {
                if (++*child == std::end(*parent->children.left)) {
                    delete child;
                    child = new Iterator(parent->children.right, 0);
                }
            }
            pos++;
            return *this;
        }
        Iterator& operator--() {
            if (child) {
                if (*child == std::begin(*parent->children.right)) {
                    delete child;
                    child = new Iterator(parent->children.left, ~parent->length);
                }
            }
            pos--;
            return *this;
        }
        char operator*() const {
            if (parent->length < 0) {
                return **child;
            }
            assert(pos < parent->string.size);
            return parent->string.content[pos];
        }
        bool operator==(const Iterator& rhs) const {
            if (parent == rhs.parent && pos == rhs.pos) {
                return true;
            }
            if (child) {
                if (*child == rhs) {
                    return true;
                }
            }
            if (rhs.child) {
                if (*this == *rhs.child) {
                    return true;
                }
            }
            return false;
        }
        inline bool operator!=(const Iterator& rhs) const {
            return !(*this == rhs);
        }
        ~Iterator() {
            if (child) {
                delete child;
            }
        }
    };
    Iterator begin() const {
        return {this, 0};
    }
    Iterator end() const {
        if (length < 0) {
            return {this, (size_t)-length};
        }
        return {this, (size_t)length};
    }
    Rope() {
        assert(16 < maxSize);
        string.content = (char*)malloc(16);
        string.size = 16;
    }
    void print();
    void insertToChild(char c, ssize_t where) {
        if (where < -length) {
            children.left->insert(c, where);
            length--;
        } else {
            children.right->insert(c, where+length);
        }
    }
    void insertToString(char c, ssize_t where) {
        if (string.size <= (size_t)length) {
            assert(string.size == (size_t)length);
            size_t newSize = std::min((size_t)maxSize, string.size*2);
            string.content = (char*)realloc(string.content, newSize);
            string.size = string.content ? newSize : 0;
        }
        std::memmove(string.content+where+1, string.content+where, length-where);
        string.content[where] = c;
        length++;
    }
    void insert(char c, ssize_t where) {
        if (length < 0) {
            insertToChild(c, where);
            return;
        }
        assert(where <= length);
        if (length < maxSize) {
            insertToString(c, where);
            return;
        }
        char tmp[maxSize];
        printf("%ld %lu %lu\n", length, maxSize, where);
        assert(length == maxSize && where <= maxSize);
        std::memcpy(tmp, string.content, length);
        delete string.content;
        children.left = new Rope();
        children.right = new Rope();
        for (ssize_t i = 0; i < where; i++) {
            children.left->insertToString(tmp[i], i);
        }
        children.right->insertToString(c, 0);
        for (ssize_t i = where; i < length; i++) {
            children.right->insertToString(tmp[i], 1+i-where);
        }
        length = -length;
    }
};

class Text{
    public:
    Rope::Iterator begin() const {
        return std::begin(rope);
    }
    Rope::Iterator end() const {
        return std::end(rope);
    }
    void insert(char c);
    void insert(const char* string);
    void print();
    // void moveRel();
    Rope::Iterator getView(int startLine, int lineCount) const;
    private:
    // char* buffer;
    // size_t bufferSize;
    uint64_t cursor;
    Rope rope;
};

#else 
#include <utility>

extern const char* untitled;

class Text{
    public:
    class Iterator{
        const char* base = 0;
        size_t gapSize = 0;
        public:
        size_t pos = 0;
        size_t cursorPos = 0;
        Iterator() = default;
        Iterator(char* buffer, size_t pos, size_t gap, size_t cursor)
         : base(buffer), gapSize(gap), pos(pos), cursorPos(cursor)
        {}
        Iterator& operator++() {
            pos++;
            return *this;
        }
        // warning: do not add more than pos-fileSize, it will break the iterator for loop
        Iterator& operator+(size_t offset) {
            pos += offset;
            return *this;
        }
        char operator*() const {
            return *(base + pos + gapSize * (pos >= cursorPos));
        }
        inline bool operator==(const Iterator& rhs) const {
            return base == rhs.base && pos == rhs.pos;
        }
        inline bool operator!=(const Iterator& rhs) const {
            return !(*this == rhs);
        }
    };
    static_assert(std::is_trivially_copyable_v<Iterator>);
    Iterator begin() const {
        return {buffer, 0, bufferSize-fileSize, cursor};
    }
    Iterator end() const {
        return {buffer, fileSize, bufferSize-fileSize, cursor};
    }
    Text();
    Text(const char* file);
    Text& operator=(Text&&);
    Text(Text&&);
    ~Text();
    void save(const char* filename) const;
    void load(const char* filename);
    void print() const;
    void insert(char c);
    void insert(const char* str);
    void del(bool wordWise = false);
    void backspace(bool wordWise = false);
    void left(bool wordWise = false);
    void right(bool wordWise = false);
    void up(std::vector<ssize_t>& newLines, ssize_t inLineOffset);
    void down(std::vector<ssize_t>& newLines, ssize_t inLineOffset);
    ssize_t home(std::vector<ssize_t>& newLines);
    void ende(std::vector<ssize_t>& newLines);
    void beginning();
    void ending();
    // void moveRel();
    std::pair<Iterator, Iterator> getView(int startLine, int lineCount) const;
    private:
    char* buffer = nullptr;
    size_t bufferSize = 0;
    uint64_t cursor = 0;
    size_t fileSize = 0;
};

#endif
