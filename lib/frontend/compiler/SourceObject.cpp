/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "SourceObject.hpp"

#include "Lexer.hpp"

template class coral66::SourceObject<coral66::Lexer::CToken>;
template class coral66::SourceObject<coral66::Lexer::PPToken>;
