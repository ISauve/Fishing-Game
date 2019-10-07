#!lua

-- Additional Linux libs: "X11", "Xxf86vm", "Xi", "Xrandr", "stdc++"

includeDirList = {
    "external",
    "external/include",
    "external/gl3w",
    "external/imgui",
    "external/lodepng"
}

libDirectories = {
    "lib"
}

if os.get() == "macosx" then
    linkLibs = {
        "cs488-framework",
        "imgui",
        "glfw3",
        "lodepng",
        "Assimp"
    }
end

if os.get() == "linux" then
    linkLibs = {
        "cs488-framework",
        "imgui",
        "glfw3",
        "GL",
        "Xinerama",
        "Xcursor",
        "Xxf86vm",
        "Xi",
        "Xrandr",
        "X11",
        "stdc++",
        "dl",
        "pthread"
    }
end

-- Build Options:
if os.get() == "macosx" then
    linkOptionList = { "-framework IOKit", "-framework Cocoa", "-framework CoreVideo", "-framework OpenGL" }
end

buildOptions = {"-std=c++14"}

solution "CS488-Projects"
    configurations { "Debug", "Release" }

    project "FishingGame"
        kind "ConsoleApp"
        language "C++"
        location "build"
        objdir "build"
        targetdir "."
        buildoptions (buildOptions)
        libdirs (libDirectories)
        links (linkLibs)
        linkoptions (linkOptionList)
        includedirs (includeDirList)
        files { "*.cpp" }

    configuration "Debug"
        defines { "DEBUG" }
        flags { "Symbols" }

    configuration "Release"
        defines { "NDEBUG" }
        flags { "Optimize" }
