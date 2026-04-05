/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "message.hpp"

llvm::raw_ostream& coral66::operator<<(llvm::raw_ostream& os, const Message& message)

{
    os << message.m_text;
    os.flush();
    return os;
}
