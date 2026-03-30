/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 30/03/2026
 */

#include "parser.hpp"

#include <algorithm>

#include "lib/frontend/common/messages/errors.hpp"
#include "parser_support.hpp"

namespace coral66::parser
{
/*
 * Section 8
 *
 * Procedures
 *
 */

std::unique_ptr<Syntax::ProcedureDeclaration> parseProcedure(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                             Context& context)
{
    if (begin != end && begin->getTokenType() == lexer::TokenType::ProcedureKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
        {
            auto name = begin;
            begin++;
            std::vector<std::unique_ptr<Syntax::Declaration>> parameters;
            if (begin != end && begin->getTokenType() == lexer::TokenType::OpenParentheses)
            {
                begin++;
                while (begin != end && begin->getTokenType() != lexer::TokenType::CloseParentheses)
                {
                    for (auto parameterDeclarations = parseDeclaration(begin, end, context);
                         auto& p : parameterDeclarations)
                        parameters.push_back(std::move(p));

                    if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
                        begin++; // NOLINT(*-branch-clone)
                    else if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                        begin++;
                    else if (begin != end && begin->getTokenType() != lexer::TokenType::CloseParentheses)
                        begin++;
                }
                if (begin != end)
                    begin++;
            }
            if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
            {
                begin++;
            }
            if (begin != end && begin->getTokenType() == lexer::TokenType::BeginKeyword)
            {
                context.enterProcedure(true);
                if (auto body = parseBlock(begin, end, context))
                {
                    if (!context.isAnswerStatementFound())
                    {
                        context.log(errors::parser::MISSING_ANSWER_STATEMENT.args(*start, context.getSourceInterface(),
                                                                                  *start));
                        DiagnosticCollector::addDiagnostic(errors::parser::MISSING_ANSWER_STATEMENT);
                    }
                    return std::make_unique<Syntax::ProcedureDeclaration>(start, begin, name, std::move(parameters),
                                                                          std::move(body), start);
                }
                context.exitProcedure();
            }
        }
        return nullptr;
    }
    return nullptr;
}
} // namespace coral66::parser
