#pragma once

#include <cstdlib>

// #ifdef DEBUG
#ifdef SDL_CHK
#error "redefining SDL_CHK"
#endif // SDL_CHK

#define SDL_CHK(X) if (!(X)) {fprintf(stderr, "Error at %s:%d: %s\n", __FILE__, __LINE__, SDL_GetError()); exit(1);}
// #endif // DEBUG

#define UNUSED(x) (void)(x)

struct Array{
    char* content;
    size_t size;
};
