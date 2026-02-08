/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser.hpp"

#include "lib/frontend/common/error_messages.hpp"
#include "lib/frontend/common/source_object.hpp"

namespace coral66::Parser
{

Context::Context(const source_interface& sourceInterface, llvm::raw_ostream* reporter)
    : m_sourceInterface(sourceInterface), m_reporter(reporter)
{
}

void Context::log(const Message& message)
{
    if (message.getSeverity() == Severity::Error)
    {
        m_errorCount++;
    }
    if (m_reporter)
    {
        *m_reporter << message;
    }
}

const source_interface& Context::getSourceInterface() const
{
    return m_sourceInterface;
}

std::size_t Context::getErrorCount() const
{
    return m_errorCount;
}

Syntax::Program parseProgram(const CSourceObject& sourceObject, llvm::raw_ostream* reporter, bool* errors)
{
    Context context(sourceObject, reporter);
    auto begin = sourceObject.data().data();
    auto end = sourceObject.data().data() + sourceObject.data().size();

    // Consume optional PROGRAM/SEGMENT header
    if (begin != end
        && (begin->getTokenType() == Lexer::TokenType::ProgramKeyword
            || begin->getTokenType() == Lexer::TokenType::SegmentKeyword))
    {
        begin++;
        if (begin != end && begin->getTokenType() == Lexer::TokenType::Identifier)
        {
            begin++;
        }
    }

    auto block = parseBlock(begin, end, context);
    if (!block && context.getErrorCount() == 0)
    {
        if (begin != end)
        {
            context.log(Errors::Parser::EXPECTED_N.args(*begin, context.getSourceInterface(),
                                                        Lexer::TokenType::BeginKeyword, *begin));
        }
    }

    if (errors)
    {
        *errors = context.getErrorCount() > 0;
    }

    return {sourceObject.data().data(), end, std::move(block)};
}

std::unique_ptr<Syntax::Block> parseBlock(Lexer::CTokenIterator& begin, Lexer::CTokenIterator end, Context& context)
{
    if (begin == end || begin->getTokenType() != Lexer::TokenType::BeginKeyword)
    {
        return nullptr;
    }
    auto blockStart = begin;
    begin++;

    std::vector<std::unique_ptr<Syntax::Declaration>> decls;
    std::vector<std::unique_ptr<Syntax::Statement>> stmts;

    while (begin != end && begin->getTokenType() != Lexer::TokenType::EndKeyword)
    {
        auto newDecls = parseDeclaration(begin, end, context);
        if (!newDecls.empty())
        {
            for (auto& d : newDecls)
                decls.push_back(std::move(d));
            continue;
        }
        if (auto stmt = parseStatement(begin, end, context))
        {
            stmts.push_back(std::move(stmt));
            continue;
        }
        begin++;
    }

    if (begin != end)
    {
        begin++;
    }

    return std::make_unique<Syntax::Block>(blockStart, begin, std::move(decls), std::move(stmts));
}

std::vector<std::unique_ptr<Syntax::Declaration>> parseDeclaration(Lexer::CTokenIterator& begin,
                                                                   Lexer::CTokenIterator end, Context& context)
{
    std::vector<std::unique_ptr<Syntax::Declaration>> result;
    if (begin == end)
    {
        return result;
    }
    auto type = begin->getTokenType();
    if (type == Lexer::TokenType::IntegerKeyword || type == Lexer::TokenType::FloatingKeyword
        || type == Lexer::TokenType::FixedKeyword)
    {
        auto typeToken = begin;
        begin++;

        if (type == Lexer::TokenType::FixedKeyword && begin != end
            && begin->getTokenType() == Lexer::TokenType::OpenParentheses)
        {
            begin++;
            while (begin != end && begin->getTokenType() != Lexer::TokenType::CloseParentheses)
                begin++;
            if (begin != end)
                begin++;
        }

        if (begin != end && begin->getTokenType() == Lexer::TokenType::ArrayKeyword)
        {
            begin++; // Consume ARRAY
            while (begin != end && begin->getTokenType() == Lexer::TokenType::Identifier)
            {
                auto id = begin;
                begin++;
                std::vector<std::pair<std::unique_ptr<Syntax::Expression>, std::unique_ptr<Syntax::Expression>>> dims;
                if (begin != end && begin->getTokenType() == Lexer::TokenType::OpenSquareBracket)
                {
                    begin++;
                    while (begin != end && begin->getTokenType() != Lexer::TokenType::CloseSquareBracket)
                    {
                        auto lower = parseExpression(begin, end, context);
                        if (begin != end && begin->getTokenType() == Lexer::TokenType::Colon)
                        {
                            begin++;
                            auto upper = parseExpression(begin, end, context);
                            dims.emplace_back(std::move(lower), std::move(upper));
                        }
                        if (begin != end && begin->getTokenType() == Lexer::TokenType::Comma)
                            begin++;
                        else
                            break;
                    }
                    if (begin != end)
                        begin++;
                }
                result.push_back(
                    std::make_unique<Syntax::ArrayDeclaration>(typeToken, begin, typeToken, id, std::move(dims)));

                if (begin != end && begin->getTokenType() == Lexer::TokenType::Comma)
                    begin++;
                else
                    break;
            }
        }
        else
        {
            std::vector<Lexer::CTokenIterator> identifiers;
            while (begin != end && begin->getTokenType() == Lexer::TokenType::Identifier)
            {
                identifiers.push_back(begin);
                begin++;
                if (begin != end && begin->getTokenType() == Lexer::TokenType::Comma)
                {
                    begin++;
                }
                else
                {
                    break;
                }
            }
            if (!identifiers.empty())
                result.push_back(
                    std::make_unique<Syntax::VariableDeclaration>(typeToken, begin, typeToken, std::move(identifiers)));
        }

        if (begin != end && begin->getTokenType() == Lexer::TokenType::SemiColon)
        {
            begin++;
        }
        return result;
    }

    if (type == Lexer::TokenType::TableKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == Lexer::TokenType::Identifier)
        {
            auto id = begin;
            begin++;
            std::unique_ptr<Syntax::Expression> width, length;
            if (begin != end && begin->getTokenType() == Lexer::TokenType::OpenSquareBracket)
            {
                begin++;
                width = parseExpression(begin, end, context);
                if (begin != end && begin->getTokenType() == Lexer::TokenType::Comma)
                {
                    begin++;
                    length = parseExpression(begin, end, context);
                }
                if (begin != end && begin->getTokenType() == Lexer::TokenType::CloseSquareBracket)
                    begin++;
            }

            std::vector<std::unique_ptr<Syntax::Declaration>> elements;
            if (begin != end && begin->getTokenType() == Lexer::TokenType::OpenSquareBracket)
            {
                begin++;
                while (begin != end && begin->getTokenType() != Lexer::TokenType::CloseSquareBracket)
                {
                    begin++;
                }
                if (begin != end)
                    begin++;
            }

            result.push_back(std::make_unique<Syntax::TableDeclaration>(start, begin, id, std::move(width),
                                                                        std::move(length), std::move(elements)));
            if (begin != end && begin->getTokenType() == Lexer::TokenType::SemiColon)
                begin++;
        }
        return result;
    }

    if (type == Lexer::TokenType::OverlayKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == Lexer::TokenType::Identifier)
        {
            auto base = begin;
            begin++;
            if (begin != end && begin->getTokenType() == Lexer::TokenType::Identifier && begin->getText() == "WITH")
            {
                begin++;
            }

            auto decls = parseDeclaration(begin, end, context);
            if (!decls.empty())
            {
                result.push_back(
                    std::make_unique<Syntax::OverlayDeclaration>(start, begin, base, base, std::move(decls[0])));
            }
        }
        return result;
    }

    if (type == Lexer::TokenType::ProcedureKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == Lexer::TokenType::Identifier)
        {
            auto name = begin;
            begin++;
            std::vector<std::unique_ptr<Syntax::Declaration>> parameters;
            if (begin != end && begin->getTokenType() == Lexer::TokenType::OpenParentheses)
            {
                begin++;
                while (begin != end && begin->getTokenType() != Lexer::TokenType::CloseParentheses)
                {
                    auto paramDecls = parseDeclaration(begin, end, context);
                    for (auto& p : paramDecls)
                        parameters.push_back(std::move(p));

                    if (begin != end && begin->getTokenType() == Lexer::TokenType::SemiColon)
                        begin++; // NOLINT(*-branch-clone)
                    else if (begin != end && begin->getTokenType() == Lexer::TokenType::Comma)
                        begin++;
                    else if (begin != end && begin->getTokenType() != Lexer::TokenType::CloseParentheses)
                        begin++;
                }
                if (begin != end)
                    begin++;
            }
            if (begin != end && begin->getTokenType() == Lexer::TokenType::SemiColon)
            {
                begin++;
            }
            if (begin != end && begin->getTokenType() == Lexer::TokenType::BeginKeyword)
            {
                if (auto body = parseBlock(begin, end, context))
                {
                    if (begin != end && begin->getTokenType() == Lexer::TokenType::SemiColon)
                        begin++;
                    result.push_back(std::make_unique<Syntax::ProcedureDeclaration>(
                        start, begin, name, std::move(parameters), std::move(body)));
                }
            }
        }
    }
    return result;
}

std::unique_ptr<Syntax::Statement> parseStatement(Lexer::CTokenIterator& begin, Lexer::CTokenIterator end,
                                                  Context& context)
{
    if (begin == end)
    {
        return nullptr;
    }
    if (begin->getTokenType() == Lexer::TokenType::Identifier)
    {
        auto start = begin;
        auto iter = begin + 1;
        if (iter != end && iter->getTokenType() == Lexer::TokenType::Assignment)
        {
            auto id = begin;
            begin++;
            begin++;
            auto expr = parseExpression(begin, end, context);
            if (begin != end && begin->getTokenType() == Lexer::TokenType::SemiColon)
            {
                begin++;
            }
            if (expr)
            {
                return std::make_unique<Syntax::AssignmentStatement>(id, begin, id, std::move(expr));
            }
        }
        else if (iter != end
                 && (iter->getTokenType() == Lexer::TokenType::OpenParentheses
                     || iter->getTokenType() == Lexer::TokenType::SemiColon))
        {
            // Call: id(...) or id;
            auto expr = parseExpression(begin, end, context);
            if (begin != end && begin->getTokenType() == Lexer::TokenType::SemiColon)
            {
                begin++;
            }
            if (expr)
            {
                return std::make_unique<Syntax::ProcedureCallStatement>(start, begin, std::move(expr));
            }
        }
        return nullptr;
    }
    if (begin->getTokenType() == Lexer::TokenType::GotoKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == Lexer::TokenType::Identifier)
        {
            auto label = begin;
            begin++;
            if (begin != end && begin->getTokenType() == Lexer::TokenType::SemiColon)
            {
                begin++;
            }
            return std::make_unique<Syntax::GotoStatement>(start, begin, label);
        }
        return nullptr;
    }
    if (begin->getTokenType() == Lexer::TokenType::IfKeyword)
    {
        auto start = begin;
        begin++;
        auto cond = parseExpression(begin, end, context);
        if (begin != end && begin->getTokenType() == Lexer::TokenType::ThenKeyword)
        {
            begin++;
            auto thenBranch = parseStatement(begin, end, context);
            std::unique_ptr<Syntax::Statement> elseBranch = nullptr;
            if (begin != end && begin->getTokenType() == Lexer::TokenType::ElseKeyword)
            {
                begin++;
                elseBranch = parseStatement(begin, end, context);
            }
            if (cond && thenBranch)
            {
                return std::make_unique<Syntax::IfStatement>(start, begin, std::move(cond), std::move(thenBranch),
                                                             std::move(elseBranch));
            }
        }
        return nullptr;
    }
    if (begin->getTokenType() == Lexer::TokenType::CodeKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == Lexer::TokenType::StringLiteral)
        {
            begin++;
            if (begin != end && begin->getTokenType() == Lexer::TokenType::SemiColon)
            {
                begin++;
            }
            return std::make_unique<Syntax::CodeStatement>(start, begin);
        }
    }

    if (begin->getTokenType() == Lexer::TokenType::ForKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == Lexer::TokenType::Identifier)
        {
            auto var = begin;
            begin++;
            if (begin != end && begin->getTokenType() == Lexer::TokenType::Assignment)
            {
                begin++;
                auto startExpr = parseExpression(begin, end, context);
                if (begin != end && begin->getTokenType() == Lexer::TokenType::StepKeyword)
                {
                    begin++;
                    auto stepExpr = parseExpression(begin, end, context);
                    if (begin != end && begin->getTokenType() == Lexer::TokenType::UntilKeyword)
                    {
                        begin++;
                        auto untilExpr = parseExpression(begin, end, context);
                        if (begin != end && begin->getTokenType() == Lexer::TokenType::DoKeyword)
                        {
                            begin++;
                            auto body = parseStatement(begin, end, context);
                            if (startExpr && stepExpr && untilExpr && body)
                            {
                                return std::make_unique<Syntax::ForStatement>(start, begin, var, std::move(startExpr),
                                                                              std::move(stepExpr), std::move(untilExpr),
                                                                              std::move(body));
                            }
                        }
                    }
                }
            }
        }
        return nullptr;
    }

    if (begin->getTokenType() == Lexer::TokenType::AnswerKeyword)
    {
        auto start = begin;
        begin++;
        auto expr = parseExpression(begin, end, context);
        if (begin != end && begin->getTokenType() == Lexer::TokenType::SemiColon)
        {
            begin++;
        }
        if (expr)
        {
            return std::make_unique<Syntax::AnswerStatement>(start, begin, std::move(expr));
        }
        return nullptr;
    }

    return nullptr;
}

std::unique_ptr<Syntax::Expression> parseExpression(Lexer::CTokenIterator& begin, Lexer::CTokenIterator end,
                                                    Context& context)
{
    if (begin == end)
    {
        return nullptr;
    }

    std::unique_ptr<Syntax::Expression> left;

    if (begin->getTokenType() == Lexer::TokenType::Identifier)
    {
        auto id = begin;
        begin++;
        // Hack: consume () for procedure calls to allow parsing as expression
        if (begin != end && begin->getTokenType() == Lexer::TokenType::OpenParentheses)
        {
            begin++;
            if (begin != end && begin->getTokenType() == Lexer::TokenType::CloseParentheses)
            {
                begin++;
            }
        }
        left = std::make_unique<Syntax::IdentifierExpression>(id, begin, id);
    }
    else if (begin->getTokenType() == Lexer::TokenType::Literal
             || begin->getTokenType() == Lexer::TokenType::StringLiteral)
    {
        auto lit = begin;
        begin++;
        left = std::make_unique<Syntax::LiteralExpression>(lit, begin, lit);
    }
    else if (begin->getTokenType() == Lexer::TokenType::BitsKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == Lexer::TokenType::OpenParentheses)
        {
            begin++;
            auto s = parseExpression(begin, end, context);
            if (begin != end && begin->getTokenType() == Lexer::TokenType::Comma)
            {
                begin++;
                auto w = parseExpression(begin, end, context);
                if (begin != end && begin->getTokenType() == Lexer::TokenType::CloseParentheses)
                {
                    begin++;
                    auto base = parseExpression(begin, end, context);
                    if (s && w && base)
                    {
                        return std::make_unique<Syntax::PartWordExpression>(start, begin, std::move(base), std::move(s),
                                                                            std::move(w));
                    }
                }
            }
        }
    }

    if (left && begin != end)
    {
        auto type = begin->getTokenType();
        if (type == Lexer::TokenType::Plus || type == Lexer::TokenType::Minus || type == Lexer::TokenType::Asterisk
            || type == Lexer::TokenType::Division)
        {
            auto op = begin;
            begin++;
            if (auto right = parseExpression(begin, end, context))
            {
                return std::make_unique<Syntax::BinaryExpression>(left->begin(), right->end(), std::move(left), op,
                                                                  std::move(right));
            }
        }
    }

    return left;
}

} // namespace coral66::Parser
