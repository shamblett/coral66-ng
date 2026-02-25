/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "preprocessor.hpp"

#include <algorithm>
#include <vector>

#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/messages/notes.hpp"
#include "lib/frontend/semantics/semantic_analysis.hpp"
#include "lib/support/scope_exit.hpp"

std::uint64_t coral66::pp::Preprocessor::getLineNumber(const std::uint32_t fileID,
                                                       const std::uint64_t offset) const noexcept
{
    CORAL66_ASSERT(fileID < m_files.size());
    auto result = std::ranges::lower_bound(m_files[fileID].starts, offset);
    return std::distance(m_files[fileID].starts.begin(), result) + (*result == offset ? 1 : 0);
}

std::uint64_t coral66::pp::Preprocessor::getLineStartOffset(std::uint32_t const fileID,
                                                            std::uint64_t const line) const noexcept
{
    CORAL66_ASSERT(fileID < m_files.size());
    CORAL66_ASSERT(line - 1 < m_files[fileID].starts.size());
    return m_files[fileID].starts[line - 1];
}

std::uint64_t coral66::pp::Preprocessor::getLineEndOffset(std::uint32_t const fileID,
                                                          std::uint64_t const line) const noexcept
{
    CORAL66_ASSERT(fileID < m_files.size());
    CORAL66_ASSERT(line < m_files[fileID].starts.size());
    return m_files[fileID].starts[line] - 1;
}

std::optional<coral66::pp::Preprocessor::Macro>
    coral66::pp::Preprocessor::parseDefineDirective(const DefineDirective& defineDirective)
{
    if (defineDirective.tokens.empty())
    {
        log(Errors::Parser::EXPECTED_N_AFTER_N.args(*defineDirective.defineToken, *this, lexer::TokenType::Identifier,
                                                    *defineDirective.defineToken));
        return {};
    }
    if (defineDirective.tokens[0].getTokenType() != lexer::TokenType::Identifier)
    {
        log(Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(defineDirective.tokens[0], *this, lexer::TokenType::Identifier,
                                                         defineDirective.tokens[0]));
        return {};
    }
    const auto* identifierPos = &defineDirective.tokens[0];
    const auto* curr = identifierPos + 1;
    const auto* end = defineDirective.tokens.data() + defineDirective.tokens.size();
    if (curr == end)
    {
        return Macro{identifierPos, std::nullopt, false, {}};
    }

    if (curr->getTokenType() != lexer::TokenType::OpenParentheses
        || curr->getOffset() != identifierPos->getOffset() + identifierPos->getLength())
    {
        if (curr->getTokenType() != lexer::TokenType::Newline
            && curr->getOffset() == identifierPos->getOffset() + identifierPos->getLength())
        {
            log(Errors::pp::WHITESPACE_REQUIRED_AFTER_OBJECT_MACRO_DEFINITION.args(*curr, *this, *curr));
        }
        return Macro{identifierPos, std::nullopt, false, {curr, end}};
    }

    // We are a function like macro from here on
    const auto* openP = curr++;
    if (curr == end)
    {
        log(Errors::Parser::EXPECTED_N.args(*openP, *this, lexer::TokenType::CloseParentheses, *openP));
        log(notes::TO_MATCH_N_HERE.args(*openP, *this, *openP));
        return {};
    }

    switch (curr->getTokenType())
    {
        case lexer::TokenType::CloseParentheses:
        {
            curr++;
            return Macro{identifierPos, {std::vector<lexer::PPToken>{}}, false, {curr, end}};
        }

        default: break;
    }

    if (curr->getTokenType() != lexer::TokenType::Identifier)
    {
        log(Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(*curr, *this, lexer::TokenType::Identifier, *curr));
        return {};
    }
    std::vector argumentList = {*curr};
    curr++;
    bool ellipse = false;
    while (curr != end && curr->getTokenType() == lexer::TokenType::Comma)
    {
        curr++;
        if (curr == end)
        {
            log(Errors::Parser::EXPECTED_N.args(*(curr - 1), *this, lexer::TokenType::Identifier, *(curr - 1)));
            return {};
        }
        if (curr->getTokenType() != lexer::TokenType::Identifier)
        {
            log(Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(*curr, *this, lexer::TokenType::Identifier, *curr));
            return {};
        }
        auto string = curr->getValue();
        auto result = std::ranges::find_if(
            argumentList, coral66::compose(coral66::bind_front(std::equal_to<>{}, string), &lexer::PPToken::getValue));
        if (result == argumentList.end())
        {
            argumentList.push_back(*(curr++));
            continue;
        }
        log(Errors::pp::REDEFINITION_OF_MACRO_PARAMETER_N.args(*curr, *this, *curr));
        log(notes::PREVIOUSLY_DECLARED_HERE.args(*result, *this, *result));
        curr++;
    }
    if (curr == end)
    {
        log(Errors::Parser::EXPECTED_N.args(*(curr - 1), *this, lexer::TokenType::CloseParentheses, *(curr - 1)));
        log(notes::TO_MATCH_N_HERE.args(*openP, *this, *openP));
        return {};
    }
    if (curr->getTokenType() != lexer::TokenType::CloseParentheses)
    {
        log(Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(*curr, *this, lexer::TokenType::CloseParentheses, *curr));
        log(notes::TO_MATCH_N_HERE.args(*openP, *this, *openP));
        return {};
    }
    curr++;
    return Macro{identifierPos, std::move(argumentList), ellipse, {curr, end}};
}

std::optional<bool> coral66::pp::Preprocessor::evaluateExpression(lexer::PPTokenIterator ifToken,
                                                                  std::span<const lexer::PPToken> tokens)
{
    std::vector<coral66::lexer::PPToken> result;
    macroSubstitute({tokens.begin(), tokens.end()},
                    coral66::bind_front(&append<coral66::lexer::PPToken>, std::ref(result)));
    bool errorsOccurred = false;
    auto ctokens =
        coral66::lexer::toCTokens(result.data(), result.data() + result.size(), *this, m_reporter, &errorsOccurred);
    if (errorsOccurred)
    {
        m_errorsOccurred = true;
        return {};
    }
    for (auto iter = ctokens.begin(); iter != ctokens.end(); ++iter)
    {
        if (iter->getTokenType() == coral66::lexer::TokenType::Literal)
        {
            switch (iter->getType())
            {
                case coral66::lexer::CToken::Type::Int:
                case coral66::lexer::CToken::Type::Long:
                case coral66::lexer::CToken::Type::LongLong:
                    iter->setType(coral66::lexer::CToken::Type::LongLong);
                    break;
                case coral66::lexer::CToken::Type::UnsignedInt:
                case coral66::lexer::CToken::Type::UnsignedLong:
                case coral66::lexer::CToken::Type::UnsignedLongLong:
                    iter->setType(coral66::lexer::CToken::Type::UnsignedLongLong);
                    break;
                default: break;
            }
            if (!std::holds_alternative<llvm::APSInt>(iter->getValue()))
            {
                continue;
            }
            auto integer = coral66::get<llvm::APSInt>(iter->getValue());
            integer = integer.extOrTrunc(64);
            iter->setValue(integer);
        }
        else if (coral66::lexer::isText(iter->getTokenType()))
        {
            if (iter->getTokenType() == coral66::lexer::TokenType::Identifier && iter->getText() == "defined")
            {
                // Neither defined nor the identifier following it (with optionally an opening parentheses
                // in between) should be converted to 0
                if (iter + 1 == ctokens.end())
                {
                    continue;
                }
                if ((iter + 1)->getTokenType() == coral66::lexer::TokenType::Identifier)
                {
                    ++iter;
                }
                else if ((iter + 1)->getTokenType() == coral66::lexer::TokenType::OpenParentheses)
                {
                    if (iter + 2 != ctokens.end()
                        && (iter + 2)->getTokenType() == coral66::lexer::TokenType::Identifier)
                    {
                        iter += 2;
                    }
                }
                continue;
            }
            bool hasLeadingWhitespace = iter->hasLeadingWhitespace();
            *iter = coral66::lexer::CToken(coral66::lexer::TokenType::Literal, iter->getOffset(), iter->getLength(),
                                           iter->getFileId(), iter->getMacroId(), llvm::APSInt(64, false),
                                           coral66::lexer::CToken::Type::LongLong);
            iter->setLeadingWhitespace(hasLeadingWhitespace);
        }
    }
    if (ctokens.empty())
    {
        if (ifToken->getValue() == "if")
        {
            CORAL66_ASSERT(!tokens.empty());
            log(coral66::Errors::pp::EXPECTED_AN_EXPRESSION_AFTER_IF.args(tokens, *this, tokens));
        }
        else
        {
            if (tokens.empty())
            {
                log(coral66::Errors::pp::EXPECTED_AN_EXPRESSION_AFTER_ELIF.args(*ifToken, *this, *ifToken));
            }
            else
            {
                log(coral66::Errors::pp::EXPECTED_AN_EXPRESSION_AFTER_ELIF_2.args(tokens, *this, tokens));
            }
        }
        return {};
    }
    const auto* begin = std::as_const(ctokens).data();
    auto context = coral66::Parser::Context(*this, m_reporter, true);
    auto tree = coral66::Parser::parseConditionalExpression(begin, ctokens.data() + ctokens.size(), context);
    if (context.getCurrentErrorCount() != 0 || !tree)
    {
        m_errorsOccurred = true;
        return {};
    }
    coral66::Semantics::SemanticAnalysis analysis(
        *this, m_reporter, &errorsOccurred, [this](std::string_view macro)
        { return macro == "__FILE__" || macro == "__LINE__" || m_defines.contains(macro); });
    auto exp = analysis.visit(*tree);
    if (errorsOccurred)
    {
        m_errorsOccurred = true;
        return {};
    }
    auto value = analysis.evaluateConstantExpression(*exp);
    if (!value)
    {
        std::ranges::for_each(value.error(), coral66::bind_front(&Preprocessor::log, this));
        return {};
    }
    return static_cast<bool>(*value);
}
}

coral66::PPSourceObject coral66::pp::preprocess(PPSourceObject&& sourceObject, llvm::raw_ostream* reporter,
                                                bool* errorsOccurred) noexcept
{
    Preprocessor preprocessor(reporter);
    preprocessor.process(std::move(sourceObject));
    if (errorsOccurred)
    {
        *errorsOccurred = preprocessor.errorsOccurred();
    }
    return {std::move(preprocessor.getResult()),
            std::move(preprocessor.getFiles()),
            {preprocessor.getSubstitutionsVector().begin(), preprocessor.getSubstitutionsVector().end()},
            {std::move(preprocessor.getIntervalMaps())}};
}
