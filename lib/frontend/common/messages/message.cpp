/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "message.hpp"

llvm::raw_ostream& coral66::operator<<(llvm::raw_ostream& os, const Message& message)

{
    if (message.getSegments().empty())
    {
        os << message.m_text;
    }
    else
    {
        for (const auto& [text, color, bold, bg] : message.getSegments())
        {
            if (color != llvm::raw_ostream::SAVEDCOLOR && os.colors_enabled())
            {
                os.changeColor(color, bold, bg);
                os << text;
                os.resetColor();
            }
            else
            {
                os << text;
            }
        }
    }
    os.flush();
    return os;
}
