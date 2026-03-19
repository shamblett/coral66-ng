/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 07/03/2026
 */

#pragma once

#include <llvm/Support/raw_ostream.h>

#include <utility>

#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/lexer/lexer.hpp"
#include "lib/frontend/lexer/lexer_support.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/parser/parser_support.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::Syntax;

struct ParsedExpr
{
    std::string source;
    std::string outStr;
    llvm::raw_string_ostream ss;
    PPSourceObject ppTokens;
    CSourceObject cTokens;
    std::unique_ptr<parser::Context> context;
    std::unique_ptr<Expression> expr;

    explicit ParsedExpr(std::string src) : source(std::move(src)), ss(outStr)
    {
        ppTokens = lexer::tokenize(source, &ss);
        cTokens = lexer::toCTokens(ppTokens, &ss);
        context = std::make_unique<coral66::parser::Context>(cTokens, &ss);

        const auto& tokens = cTokens.data();
        const lexer::CToken* begin = tokens.data();
        const lexer::CToken* end = tokens.data() + tokens.size();

        while (begin != end && (end - 1)->getTokenType() == lexer::TokenType::Newline)
        {
            end--;
        }

        parser::DiagnosticCollector::clearDiagnostics();
        expr = parser::parseExpression(begin, end, *context);
        REQUIRE(expr != nullptr);
        REQUIRE(begin == end);
    }
};
