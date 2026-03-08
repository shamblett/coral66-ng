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

std::pair<std::optional<coral66::pp::Preprocessor::Macro>, const coral66::lexer::PPToken*>
    coral66::pp::Preprocessor::parseDefineDirective(const DefineDirective& defineDirective)
{
    if (defineDirective.tokens.empty())
    {
        log(errors::Parser::EXPECTED_N_AFTER_N.args(*defineDirective.defineToken, *this, lexer::TokenType::Identifier,
                                                    *defineDirective.defineToken));
        return {{}, nullptr};
    }
    if (defineDirective.tokens[0].getTokenType() != lexer::TokenType::Identifier)
    {
        log(errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(defineDirective.tokens[0], *this, lexer::TokenType::Identifier,
                                                         defineDirective.tokens[0]));
        return {{}, nullptr};
    }
    const auto* identifierPos = &defineDirective.tokens[0];
    const auto* curr = identifierPos + 1;
    const auto* end = defineDirective.tokens.data() + defineDirective.tokens.size();
    if (curr == end)
    {
        return {Macro{identifierPos, {}}, curr};
    }

    if (curr->getTokenType() != lexer::TokenType::OpenParentheses
        || curr->getOffset() != identifierPos->getOffset() + identifierPos->getLength())
    {
        if (curr->getTokenType() != lexer::TokenType::Newline
            && curr->getOffset() == identifierPos->getOffset() + identifierPos->getLength())
        {
            log(errors::pp::WHITESPACE_REQUIRED_AFTER_OBJECT_MACRO_DEFINITION.args(*curr, *this, *curr));
        }
        return {Macro{identifierPos, std::nullopt}, curr};
    }

    // We are a function like macro from here on
    const auto* openP = curr++;
    if (curr == end)
    {
        log(errors::Parser::EXPECTED_N.args(*openP, *this, lexer::TokenType::CloseParentheses, *openP));
        log(notes::TO_MATCH_N_HERE.args(*openP, *this, *openP));
        return {{}, nullptr};
    }

    switch (curr->getTokenType())
    {
        case lexer::TokenType::CloseParentheses:
        {
            return {Macro{identifierPos, {std::vector<lexer::PPToken>{}}}, curr + 1};
        }

        default: break;
    }

    if (curr->getTokenType() != lexer::TokenType::Identifier)
    {
        log(errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(*curr, *this, lexer::TokenType::Identifier, *curr));
        return {{}, nullptr};
    }
    std::vector argumentList = {*curr};
    curr++;
    while (curr != end && curr->getTokenType() == lexer::TokenType::Comma)
    {
        curr++;
        if (curr == end)
        {
            log(errors::Parser::EXPECTED_N.args(*(curr - 1), *this, lexer::TokenType::Identifier, *(curr - 1)));
            return {{}, nullptr};
        }
        if (curr->getTokenType() != lexer::TokenType::Identifier)
        {
            log(errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(*curr, *this, lexer::TokenType::Identifier, *curr));
            return {{}, nullptr};
        }
        auto string = curr->getValue();
        auto result = std::ranges::find_if(
            argumentList, coral66::compose(coral66::bind_front(std::equal_to<>{}, string), &lexer::PPToken::getValue));
        if (result == argumentList.end())
        {
            argumentList.push_back(*(curr++));
            continue;
        }
        log(errors::pp::REDEFINITION_OF_MACRO_PARAMETER_N.args(*curr, *this, *curr));
        log(notes::PREVIOUSLY_DECLARED_HERE.args(*result, *this, *result));
        curr++;
    }
    if (curr == end)
    {
        log(errors::Parser::EXPECTED_N.args(*(curr - 1), *this, lexer::TokenType::CloseParentheses, *(curr - 1)));
        log(notes::TO_MATCH_N_HERE.args(*openP, *this, *openP));
        return {{}, nullptr};
    }
    if (curr->getTokenType() != lexer::TokenType::CloseParentheses)
    {
        log(errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(*curr, *this, lexer::TokenType::CloseParentheses, *curr));
        log(notes::TO_MATCH_N_HERE.args(*openP, *this, *openP));
        return {{}, nullptr};
    }

    return {Macro{identifierPos, std::move(argumentList)}, curr + 1};
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
        if (iter->getTokenType() == lexer::TokenType::Literal)
        {
            auto integer = coral66::get<llvm::APSInt>(iter->getValue());
            integer = integer.extOrTrunc(64);
            iter->setValue(integer);
        }
        else if (coral66::lexer::isText(iter->getTokenType()))
        {
            if (iter->getTokenType() == lexer::TokenType::Identifier
                && iter->getText() == lexer::tokenName(lexer::TokenType::DefineKeyword))
            {
                if (iter + 1 == ctokens.end())
                {
                    continue;
                }
                if ((iter + 1)->getTokenType() == lexer::TokenType::Identifier)
                {
                    ++iter;
                }
                else if ((iter + 1)->getTokenType() == lexer::TokenType::OpenParentheses)
                {
                    if (iter + 2 != ctokens.end() && (iter + 2)->getTokenType() == lexer::TokenType::Identifier)
                    {
                        iter += 2;
                    }
                }
                continue;
            }
            bool hasLeadingWhitespace = iter->hasLeadingWhitespace();
            *iter = lexer::CToken(lexer::TokenType::Literal, iter->getOffset(), iter->getLength(), iter->getFileId(),
                                  iter->getMacroId(), llvm::APSInt(64, false), lexer::CToken::Type::Integer);
            iter->setLeadingWhitespace(hasLeadingWhitespace);
        }
    }
    if (ctokens.empty())
    {
        if (tokens.empty())
        {
            log(errors::pp::EXPECTED_AN_EXPRESSION_AFTER_ELIF.args(*ifToken, *this, *ifToken));
        }
        else
        {
            log(errors::pp::EXPECTED_AN_EXPRESSION_AFTER_ELIF_2.args(tokens, *this, tokens));
        }
        return {};
    }
    return {};
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
