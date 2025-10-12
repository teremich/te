#pragma once

#include <cstdlib>
#include <chrono>
#include <cstring>
#include <cassert>

// #ifdef DEBUG
#ifdef SDL_CHK
#error "redefining SDL_CHK"
#endif // SDL_CHK

#define SDL_CHK(X) if (!(X)) {fprintf(stderr, "Error at %s:%d: %s\n", __FILE__, __LINE__, SDL_GetError()); exit(1);}
// #endif // DEBUG

#define UNUSED(x) (void)(x)

template <class Resolution = std::chrono::duration<float, std::milli>>
requires requires(Resolution r) {
    {std::chrono::duration{r}} -> std::same_as<Resolution>;
}
struct Timer{
    const char* name = nullptr;
    const std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    using resolution = Resolution;
    Timer() = default;
    explicit Timer(const char* name) : name(name) {}
    ~Timer() {
        const auto end = std::chrono::high_resolution_clock::now();
        const auto delta = end-start;
        printf(
            "%s: %f\n",
            name,
            delta.count()
            * std::chrono::high_resolution_clock::period::num * static_cast<Resolution::rep>(Resolution::period::den)
            / std::chrono::high_resolution_clock::period::den
            / Resolution::period::num
        );
    }
};

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
    size_t push(T&& moveFrom) {
        if (size == capacity) {
            capacity = capacity * 2 + 1;
            items = (T*) realloc(items, capacity*sizeof(T));
            // std::memset(items+size, 0, sizeof(T)*(capacity-size));
        }
        new(&items[size]) T();
        items[size] = std::move(moveFrom);
        return size++;
    }
    T&& pop() {
        assert(size);
        size--;
        return std::move(items[size]);
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
            for (size_t i = 0; i < size; i++) {
                items[i].~T();
            }
            free(items);
        }
    }
};

static inline bool isWhiteSpace(char c) {
    //   horizonal tab| line feed  |vertical tab| form feed |carriage return| space
    return 0x09 == c || 0x0A == c || 0x0B == c || 0x0C == c || 0x0D == c || 0x20 == c;
}
