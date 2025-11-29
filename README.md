# Te
This is my Text Editor.
Now I know that Text Rendering doesn't like me.

## Getting Started
after initializing the submodules build SDL_ttf
```sh
te/vendor/SDL_ttf$ cmake -S . -B build -DBUILD_SHARED_LIBS=OFF
te/vendor/SDL_ttf$ cmake --build build
```
then use CMake like usual to build this project
```sh
te$ cmake -S . -B build
te$ cmake --build build
```
you should now be able to use `bin/Editor`
