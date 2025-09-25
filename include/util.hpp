#pragma once

#include <cstdlib>

// #ifdef DEBUG
#ifdef SDL_CHK
#error "redefining SDL_CHK"
#endif // SDL_CHK

#define SDL_CHK(X) if (!(X)) {fprintf(stderr, "Error at %s:%d: %s\n", __FILE__, __LINE__, SDL_GetError()); exit(1);}
// #endif // DEBUG

#define UNUSED(x) (void)(x)

template <typename T>
struct Array{
    T* content;
    size_t size;
};

template <typename T>
struct List{
    T* items = nullptr;
    size_t size = 0;
    size_t capacity = 0;
    void push(T&& moveFrom) {
        if (size == capacity) {
            capacity = capacity * 2 + 1;
            items = (T*) realloc(items, capacity*sizeof(T));
        }
        items[size] = moveFrom;
        size++;
    }
    T&& pop() {
        if (!size) {
            return {};
        }
        size--;
        return items[size];
    }
    void clear() {
        free(items);
        items = nullptr;
        size = 0;
        capacity = 0;
    }
    List& operator=(const List&) = delete;
    List& operator=(List&& moveFrom) {
        this->~List();
        items = moveFrom.items;
        size = moveFrom.size;
        capacity = moveFrom.capacity;
        moveFrom.items = nullptr;
        moveFrom.size = 0;
        moveFrom.capacity = 0;
        return *this;
    }
    List(const List& copyFrom) {
        items = malloc(copyFrom.capacity * sizeof(T));
        capacity = copyFrom.capacity;
        size = copyFrom.size;
        for (size_t i = 0; i < size; i++) {
            items[i] = copyFrom.items[i];
        }
    }
    List(List&& moveFrom) {
        items = moveFrom.items;
        size = moveFrom.size;
        capacity = moveFrom.capacity;
        moveFrom.items = nullptr;
        moveFrom.size = 0;
        moveFrom.capacity = 0;
    }
    List() = default;
    ~List() {
        if (items) {
            free(items);
        }
    }
};
