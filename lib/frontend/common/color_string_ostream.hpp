/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 05/04/2026
 */

#pragma once
#include <llvm/Support/raw_ostream.h>

#include <string>

namespace coral66::detail
{

/**
 *
 * LLVM raw_string_stream override for diagnostic colour handling.
 *
 */
class ColorStringOstream : public llvm::raw_string_ostream
{
public:
    explicit ColorStringOstream(std::string& O) : llvm::raw_string_ostream(O)
    {
        // Only actually emit color codes if enable_colors(true) was called AND has_colors() allows it.
        // Or in tests we might want to suppress it entirely or strip ANSI codes.
    }

    [[nodiscard]] bool has_colors() const override
    {
        return true;
    } // To let WithColor use color

    llvm::raw_ostream& changeColor(const enum Colors Color, bool Bold = false, const bool BG = false) override
    { // NOLINT(*-default-arguments)
        if (!colors_enabled())
            return *this;

        const char* color_code = "";
        if (BG)
        {
            switch (Color)
            {
                case BLACK: color_code = "\033[40m"; break;
                case RED: color_code = "\033[41m"; break;
                case GREEN: color_code = "\033[42m"; break;
                case YELLOW: color_code = "\033[43m"; break;
                case BLUE: color_code = "\033[44m"; break;
                case MAGENTA: color_code = "\033[45m"; break;
                case CYAN: color_code = "\033[46m"; break;
                case WHITE: color_code = "\033[47m"; break;
                case SAVEDCOLOR: break;
                case RESET: return resetColor();
                default: break;
            }
        }
        else
        {
            switch (Color)
            {
                case BLACK: color_code = Bold ? "\033[1;30m" : "\033[0;30m"; break;
                case RED: color_code = Bold ? "\033[1;31m" : "\033[0;31m"; break;
                case GREEN: color_code = Bold ? "\033[1;32m" : "\033[0;32m"; break;
                case YELLOW: color_code = Bold ? "\033[1;33m" : "\033[0;33m"; break;
                case BLUE: color_code = Bold ? "\033[1;34m" : "\033[0;34m"; break;
                case MAGENTA: color_code = Bold ? "\033[1;35m" : "\033[0;35m"; break;
                case CYAN: color_code = Bold ? "\033[1;36m" : "\033[0;36m"; break;
                case WHITE: color_code = Bold ? "\033[1;37m" : "\033[0;37m"; break;
                case SAVEDCOLOR: break;
                case RESET: return resetColor();
                default: break;
            }
        }
        *this << color_code;
        return *this;
    }

    llvm::raw_ostream& resetColor() override
    {
        if (colors_enabled())
            *this << "\033[0m";
        return *this;
    }

    llvm::raw_ostream& reverseColor() override
    {
        if (colors_enabled())
            *this << "\033[7m";
        return *this;
    }
};

} // namespace coral66::detail
