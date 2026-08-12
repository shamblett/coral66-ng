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
        for (const auto& segment : message.getSegments())
        {
            if (segment.color != llvm::raw_ostream::SAVEDCOLOR && os.colors_enabled())
            {
                os.changeColor(segment.color, segment.bold, segment.bg);
                os << segment.text;
                os.resetColor();
            }
            else
            {
                os << segment.text;
            }
        }
    }
    os.flush();
    return os;
}
