/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 07/03/2026
 */

#pragma once

#include <llvm/Support/raw_ostream.h>

#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/lexer/lexer.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

template <typename Matcher>
void treeProduces(std::string source, Matcher const& matches)
{
    std::string string;
    llvm::raw_string_ostream ss(string);
    coral66::PPSourceObject tokens;
    tokens = coral66::lexer::tokenize(std::move(source), &ss);
    CHECK_THAT(string, Catch::Matchers::Equals(""));
    string.clear();
    auto cTokens = coral66::lexer::toCTokens(tokens, &ss);
    CHECK_THAT(string, Catch::Matchers::Equals(""));
    string.clear();
    auto tree = coral66::parser::parseProgram(cTokens, &ss);
    CHECK_THAT(string, matches);
}
