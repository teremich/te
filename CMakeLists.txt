cmake_minimum_required(VERSION 3.22)
project(TextEditor LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
# Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

if(
    NOT EXISTS "${PROJECT_SOURCE_DIR}/vendor/SDL/CMakeLists.txt"
    OR NOT EXISTS "${PROJECT_SOURCE_DIR}/vendor/SDL_ttf/CMakeLists.txt"
)
    message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

option(SDL_STATIC ON)
add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

option(BUILD_SHARED_LIBS OFF)
add_subdirectory(vendor/SDL_ttf EXCLUDE_FROM_ALL)
link_directories(
    vendor/SDL_ttf/build/debug/
)
include_directories(${SDL3_INCLUDE_FILES})
add_compile_options(-march=native -fsanitize=address -fvisibility=hidden)
add_link_options(-fsanitize=address -flto -fvisibility=hidden)
add_executable(Editor
    src/main.cc
    src/editor.cc
    src/explorerSection.cc
    src/text.cc
)
target_include_directories(Editor PRIVATE
    vendor/SDL_ttf/include/
    include/
    src/
)
target_link_libraries(Editor PRIVATE
    png
    z
    m
    SDL3::SDL3-static
    SDL3_ttf
)