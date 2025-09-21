#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <util.hpp>
#include <cassert>

#define ROPE 1

#if ROPE

struct Rope{
    // zero is invalid
    // positive means it is a leaf
    // negative means its absolute value is the sum of all sizes of the leafs to the left of it
    ssize_t length;
    static const size_t maxSize = 1024;
    union{
        struct {
            Rope *left, *right;
        } children;
        char *string;
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
                if ((signed)pos < -parent->length) {
                    child = new Iterator(parent->children.left, pos);
                } else {
                    child = new Iterator(parent->children.right, pos+parent->length);
                }
            } else {
                assert((signed)pos <= parent->length);
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
        char operator*() const {
            if (parent->length < 0) {
                return **child;
            }
            return parent->string[pos];
        }
        bool operator==(const Iterator& rhs) const {
            if (parent == rhs.parent && pos == rhs.pos) {
                return true;
            }
            if (child) {
                if (*child == rhs) {
                    printf("special case found\n");
                    return true;
                }
            }
            if (rhs.child) {
                if (*this == *rhs.child) {
                    printf("special case found\n");
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
            return {this, (unsigned)-length};
        }
        return {this, (unsigned)length};
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
    void insert(char c, size_t where);
    // void moveRel();
    Rope::Iterator getView(int startLine, int lineCount) const;
    private:
    char* buffer;
    size_t bufferSize;
    uint64_t cursor;
    Rope rope;
};

#else 


class Text{
    public:
    class Iterator{
        const char* base = 0;
        size_t pos = 0;
        size_t gapSize = 0;
        size_t cursorPos = 0;
        public:
        Iterator() = default;
        Iterator(char* buffer, size_t pos, size_t gap, size_t cursor)
         : base(buffer), pos(pos), gapSize(gap), cursorPos(cursor)
        {}
        Iterator& operator++() {
            pos++;
            return *this;
        }
        char operator*() const {
            return *(base + gapSize * (pos >= cursorPos));
        }
        inline bool operator==(const Iterator& rhs) const {
            return base == rhs.base && pos == rhs.pos;
        }
        inline bool operator!=(const Iterator& rhs) const {
            return !(*this == rhs);
        }
    };
    Iterator begin() const {
        return {buffer, 0, bufferSize-fileSize, cursor};
    }
    Iterator end() const {
        return {buffer, fileSize, bufferSize-fileSize, cursor};
    }
    void insert(char c, size_t where);
    // void moveRel();
    Iterator getView(int startLine, int lineCount) const;
    private:
    char* buffer;
    size_t bufferSize;
    uint64_t cursor;
    size_t fileSize;
};

#endif
