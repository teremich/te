workspace "TextEditor"
    configurations {"debug", "release", "test", "dist"}
    preferredtoolarchitecture "x86_64"
    startproject "Editor"
    language "C++"
    cppdialect "C++latest"
    targetdir "bin/%{cfg.buildcfg}"
    objdir "build/%{cfg.buildcfg}"
    warnings "Extra"
    architecture "x64"

project "Editor"
    kind "ConsoleApp"
    dependson {
        "SDL",
    }
    exceptionhandling "Off"
    files {
        "src/main.cc",
        "src/editor.cc",
    }
    libdirs {
        "vendor/SDL/build/%{cfg.buildcfg}",
        "vendor/SDL_ttf/build/%{cfg.buildcfg}",
        "vendor/SDL_ttf/external/freetype/build/",
        "vendor/SDL_ttf/external/harfbuzz/build/",
        "/lib/x86_64-linux-gnu/",
    }
    links {
        "SDL3:static",
        "SDL3_ttf:shared",
        "png:shared",
        "z:shared",
        "m:shared",
    }
    includedirs {
        "/usr/include/",
        "include/",
        "src/",
        "vendor/SDL/include/",
        "vendor/SDL_ttf/include/",
    }
    filter "configurations:debug"
        defines {"DEBUG"}
        symbols "On"
        runtime "Debug"
    
    filter "configurations:test"
        buildoptions {"-fsanitize=address"}
        linkoptions {"-fsanitize=address"}
        defines {"DEBUG", "TEST"}
        flags {
            "FatalWarnings",
            "ShadowedVariables",
            "UndefinedIdentifiers",
        }
        runtime "Debug"
        symbols "On"

    filter "configurations:release"
        flags {
            "FatalWarnings",
            "ShadowedVariables",
            "UndefinedIdentifiers",
        }
        optimize "On"
        symbols "On"
        runtime "Release"
    
    filter "configurations:dist"
        flags {
            "FatalWarnings",
            "ShadowedVariables",
            "UndefinedIdentifiers",
        }
        optimize "On"
        runtime "Release"

include "vendor/SDL"
include "vendor/SDL_ttf"
