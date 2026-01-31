#pragma once

#include <string>
#include <string_view>

/***
* Purpose: Provides common Ansi escape codes to format the console.
***/
namespace Ansi {

    // Ansi Control Sequence Inducer
    constexpr std::string_view CSI = "\x1b[";

    // Basic text styling
    constexpr std::string_view Reset = "\x1b[0m";
    constexpr std::string_view Bold = "\x1b[1m";
    constexpr std::string_view Dim = "\x1b[2m";
    constexpr std::string_view Italic = "\x1b[3m";
    constexpr std::string_view Underline = "\x1b[4m";
    constexpr std::string_view Blink = "\x1b[5m";
    constexpr std::string_view Inverse = "\x1b[7m";
    constexpr std::string_view Hidden = "\x1b[8m";
    constexpr std::string_view Strike = "\x1b[9m";

    // Enum to organize text colors
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

    /***
    * Purpose: Format the text color
    * Paramaters: Enum text color.
    * Result: ANSI escape code that changes the text color to the parameter color.
    ***/
    std::string fg(Color c);

    /***
    * Purpose: Format the background color.
    * : Enum background color.
    * Result: ANSI escape code that changes the background color to parameter color.
    ***/
    std::string bg(BgColor c);

    /***
    * Purpose: Format the text color using an 8-bit lookup table.
    * Parameters: Integer index (0-255).
    * Result: ANSI escape code for the specific 256-color palette foreground.
    ***/
    std::string fg256(int idx);

    /***
    * Purpose: Format the background color using an 8-bit lookup table.
    * Parameters: Integer index (0-255).
    * Result: ANSI escape code for the specific 256-color palette background.
    ***/
    std::string bg256(int idx);

    /***
    * Purpose: Format the text color using TrueColor (RGB).
    * Parameters: Integer values for red, green, and blue (0-255).
    * Result: ANSI escape code for the specific RGB foreground color.
    ***/
    std::string fgRgb(int r, int g, int b);

    /***
    * Purpose: Format the background color using TrueColor (RGB).
    * Parameters: Integer values for red, green, and blue (0-255).
    * Result: ANSI escape code for the specific RGB background color.
    ***/
    std::string bgRgb(int r, int g, int b);

    /***
    * Purpose: Move the cursor position upwards.
    * Parameters: Integer number of cells to move.
    * Result: ANSI escape code to move the cursor up n rows.
    ***/
    std::string moveUp(int n);

    /***
    * Purpose: Move the cursor position downwards.
    * Parameters: Integer number of cells to move.
    * Result: ANSI escape code to move the cursor down n rows.
    ***/
    std::string moveDown(int n);

    /***
    * Purpose: Move the cursor position forward (right).
    * Parameters: Integer number of cells to move.
    * Result: ANSI escape code to move the cursor right n columns.
    ***/
    std::string moveForward(int n);

    /***
    * Purpose: Move the cursor position backward (left).
    * Parameters: Integer number of cells to move.
    * Result: ANSI escape code to move the cursor left n columns.
    ***/
    std::string moveBack(int n);

    /***
    * Purpose: Set the absolute cursor position.
    * Parameters: Integer row and integer column.
    * Result: ANSI escape code to move the cursor to specific coordinates (r, c).
    ***/
    std::string moveTo(int r, int c);

    /***
    * Purpose: Save the current cursor position.
    * Parameters: None.
    * Result: ANSI escape code that stores the current cursor state.
    ***/
    std::string saveCursor();

    /***
    * Purpose: Restore the last saved cursor position.
    * Parameters: None.
    * Result: ANSI escape code that returns the cursor to the saved state.
    ***/
    std::string restoreCursor();

    // Visibility
    constexpr std::string_view hideCursor = "\033[?25l";
    constexpr std::string_view showCursor = "\033[?25h";

    // Clear screen and clear screen after the print and clear screen before the print.
    constexpr std::string_view clearScreen = "\x1b[2J";
    constexpr std::string_view clearScreenAfter = "\x1b[0J";
    constexpr std::string_view clearScreenBefore = "\x1b[1J";

    // Clear line and clear line after the print and clear line before the print.
    constexpr std::string_view clearLine = "\x1b[2K";
    constexpr std::string_view clearLineAfter = "\x1b[0K";
    constexpr std::string_view clearLineBefore = "\x1b[1K";

}