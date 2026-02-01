/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "Message.hpp"

llvm::raw_ostream& coral66::operator<<(llvm::raw_ostream& os, const coral66::Message& message)

{
    os << message.m_text;
    os.flush();
    return os;
}
