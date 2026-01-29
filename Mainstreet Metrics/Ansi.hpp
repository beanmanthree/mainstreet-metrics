#pragma once

#include <string>
#include <string_view>

/***
Purpose: Provides common Ansi escape codes to format the console.
***/
namespace Ansi {

    // Ansi Control Sequence Inducer.
    constexpr std::string_view CSI = "\x1b[";

    // Basic text styling.
    constexpr std::string_view Reset = "\x1b[0m";
    constexpr std::string_view Bold = "\x1b[1m";
    constexpr std::string_view Dim = "\x1b[2m";
    constexpr std::string_view Italic = "\x1b[3m";
    constexpr std::string_view Underline = "\x1b[4m";
    constexpr std::string_view Blink = "\x1b[5m";
    constexpr std::string_view Inverse = "\x1b[7m";
    constexpr std::string_view Hidden = "\x1b[8m";
    constexpr std::string_view Strike = "\x1b[9m";

    // Enum to organize text colors.
    enum class Color : int {
        Default = 39,
        Black = 30,
        Red = 31,
        Green = 32,
        Yellow = 33,
        Blue = 34,
        Magenta = 35,
        Cyan = 36,
        White = 37,

        BrightBlack = 90,
        BrightRed = 91,
        BrightGreen = 92,
        BrightYellow = 93,
        BrightBlue = 94,
        BrightMagenta = 95,
        BrightCyan = 96,
        BrightWhite = 97
    };

    // Enum to organize text background colors.
    enum class BgColor : int {
        Default = 49,
        Black = 40,
        Red = 41,
        Green = 42,
        Yellow = 43,
        Blue = 44,
        Magenta = 45,
        Cyan = 46,
        White = 47,

        BrightBlack = 100,
        BrightRed = 101,
        BrightGreen = 102,
        BrightYellow = 103,
        BrightBlue = 104,
        BrightMagenta = 105,
        BrightCyan = 106,
        BrightWhite = 107
    };

    std::string fg(Color c);
    std::string bg(BgColor c);
    std::string fg256(int idx);
    std::string bg256(int idx);
    std::string fgRgb(int r, int g, int b);
    std::string bgRgb(int r, int g, int b);

    std::string moveUp(int n);
    std::string moveDown(int n);
    std::string moveForward(int n);
    std::string moveBack(int n);

    std::string moveTo(int r, int c);

    std::string saveCursor();
    std::string restoreCursor();

    constexpr std::string_view clearScreen = "\x1b[2J";
    constexpr std::string_view clearScreenAfter = "\x1b[0J";
    constexpr std::string_view clearScreenBefore = "\x1b[1J";

    constexpr std::string_view clearLine = "\x1b[2K";
    constexpr std::string_view clearLineAfter = "\x1b[0K";
    constexpr std::string_view clearLineBefore = "\x1b[1K";
}