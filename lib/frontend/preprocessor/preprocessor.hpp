/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <llvm/Support/raw_ostream.h>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/messages/notes.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "preprocessor_parser.hpp"

namespace coral66::pp
{
struct Options
{
    std::vector<std::pair<std::string, std::string>> additionalMacros;
};

template <class T>
std::vector<T>& append(std::vector<T>& lhs, std::vector<T>&& rhs)
{
    lhs.insert(lhs.end(), std::move_iterator(rhs.begin()), std::move_iterator(rhs.end()));
    return lhs;
}

class Preprocessor final : coral66::PPSourceInterface
{
    llvm::raw_ostream* m_reporter;
    std::uint32_t m_macroID = 0;
    const coral66::pp::Options& m_ppOptions;
    std::vector<coral66::lexer::PPToken> m_result;
    std::vector<coral66::Source::PPRecord> m_substitutions{1};

    struct Macro
    {
        coral66::lexer::PPTokenIterator identifierPos;
        std::optional<std::vector<coral66::lexer::PPToken>> argumentList;
        bool hasEllipse;
        std::span<const coral66::lexer::PPToken> replacement;
    };
    std::unordered_map<std::string_view, Macro> m_defines;

    std::vector<std::unordered_set<coral66::lexer::PPTokenIterator>> m_disabledMacros{1};
    std::vector<coral66::Source::File> m_files;
    std::vector<std::vector<coral66::lexer::PPToken>> m_fileTokens;
    std::vector<coral66::lexer::IntervalMap> m_intervalMaps;
    bool m_errorsOccurred = false;
    bool m_visitingScratchPad = true;

    void pushLine(std::vector<coral66::lexer::PPToken>&& tokens)
    {
        if (m_visitingScratchPad)
        {
            return;
        }
        append(m_result, std::move(tokens));
    }

    // ReSharper disable once CppDFAUnreachableFunctionCall
    bool log(const coral66::Message& message)
    {
        if (m_reporter)
        {
            *m_reporter << message;
        }
        if (message.getSeverity() == coral66::Severity::Error)
        {
            m_errorsOccurred = true;
        }
        return message.getSeverity() != coral66::Severity::None;
    }

    template <class F>
    void macroSubstitute(std::vector<coral66::lexer::PPToken>&& tokens, F lineOutput)
    {
        if (tokens.empty())
        {
            return;
        }
        std::vector<coral66::lexer::PPToken> output;
        auto* start = tokens.data();

        for (auto* iter = tokens.data(); iter != tokens.data() + tokens.size();)
        {
            if (iter->getTokenType() == coral66::lexer::TokenType::Newline)
            {
                iter++;
                if (iter == tokens.data() + tokens.size())
                {
                    break;
                }
                output.insert(output.end(), std::move_iterator(start), std::move_iterator(iter - 1));
                start = iter;
                lineOutput(std::move(output));
                output.clear();
                continue;
            }
            if (iter->getTokenType() != coral66::lexer::TokenType::Identifier)
            {
                iter++;
                continue;
            }

            auto name = iter->getValue();
            auto result = m_defines.find(name);
            if (result == m_defines.end())
            {
                iter++;
                continue;
            }

            // Expand Macro
            output.insert(output.end(), std::move_iterator(start), std::move_iterator(iter));

            auto i = ++m_macroID;
            m_substitutions.emplace_back(
                coral66::Source::Substitution{*result->second.identifierPos, *iter, {}, false});

            std::vector<coral66::lexer::PPToken> replacement(result->second.replacement.begin(),
                                                             result->second.replacement.end());
            for (auto& tok : replacement)
            {
                tok.setMacroId(i);
            }

            // Recursive substitution
            output.insert(output.end(), std::move_iterator(replacement.begin()), std::move_iterator(replacement.end()));

            iter++;
            start = iter;
        }
        output.insert(output.end(), std::move_iterator(start), std::move_iterator(tokens.data() + tokens.size()));
        lineOutput(std::move(output));
    }

    static std::string escapeString(std::string_view input)
    {
        std::string result;
        result.reserve(input.size());
        for (auto& iter : input)
        {
            switch (iter)
            {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '?': result += "\\?"; break;
                default: result += iter;
            }
        }
        return {input.begin(), input.end()};
    }

    std::optional<Macro> parseDefineDirective(const coral66::pp::DefineDirective& defineDirective)
    {
        if (defineDirective.tokens.empty())
        {
            log(coral66::Errors::Parser::EXPECTED_N_AFTER_N.args(
                *defineDirective.defineToken, *this, coral66::lexer::TokenType::Identifier, *defineDirective.defineToken));
            return {};
        }
        if (defineDirective.tokens[0].getTokenType() != coral66::lexer::TokenType::Identifier)
        {
            log(coral66::Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(
                defineDirective.tokens[0], *this, coral66::lexer::TokenType::Identifier, defineDirective.tokens[0]));
            return {};
        }
        const auto* identifierPos = &defineDirective.tokens[0];
        const auto* curr = identifierPos + 1;
        const auto* end = defineDirective.tokens.data() + defineDirective.tokens.size();
        if (curr == end)
        {
            return Macro{identifierPos, std::nullopt, false, {}};
        }

        if (curr->getTokenType() != coral66::lexer::TokenType::OpenParentheses
            || curr->getOffset() != identifierPos->getOffset() + identifierPos->getLength())
        {
            if (curr->getTokenType() != coral66::lexer::TokenType::Newline
                && curr->getOffset() == identifierPos->getOffset() + identifierPos->getLength())
            {
                log(coral66::Errors::pp::WHITESPACE_REQUIRED_AFTER_OBJECT_MACRO_DEFINITION.args(*curr, *this, *curr));
            return Macro{identifierPos, std::nullopt, false, {curr, end}};
        }

        // We are a function like macro from here on
        const auto* openP = curr++;
        if (curr == end)
        {
            log(coral66::Errors::Parser::EXPECTED_N.args(*openP, *this, coral66::lexer::TokenType::CloseParentheses, *openP));
            log(coral66::notes::TO_MATCH_N_HERE.args(*openP, *this, *openP));
            return {};
        }

        switch (curr->getTokenType())
        {
            case coral66::lexer::TokenType::CloseParentheses:
            {
                curr++;
                return Macro{identifierPos, {std::vector<coral66::lexer::PPToken>{}}, false, {curr, end}};
            }
            case coral66::lexer::TokenType::Ellipse:
            {
                auto argumentList = std::vector{*curr};
                curr++;
                if (curr == end)
                {
                    log(coral66::Errors::Parser::EXPECTED_N_AFTER_N.args(
                        *(curr - 1), *this, coral66::lexer::TokenType::CloseParentheses, *(curr - 1)));
                    log(coral66::Notes::TO_MATCH_N_HERE.args(*openP, *this, *openP));
                    return {};
                }
                if (curr->getTokenType() != coral66::lexer::TokenType::CloseParentheses)
                {
                    log(coral66::Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(
                        *curr, *this, coral66::lexer::TokenType::CloseParentheses, *curr));
                    log(coral66::Notes::TO_MATCH_N_HERE.args(*openP, *this, *openP));
                    return {};
                }
                return Macro{identifierPos, std::move(argumentList), true, {curr + 1, end}};
            }
            default: break;
        }

        if (curr->getTokenType() != coral66::lexer::TokenType::Identifier)
        {
            log(coral66::Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(*curr, *this, coral66::lexer::TokenType::Identifier,
                                                                  *curr));
            return {};
        }
        std::vector<coral66::lexer::PPToken> argumentList = {*curr};
        curr++;
        bool ellipse = false;
        while (curr != end && curr->getTokenType() == coral66::lexer::TokenType::Comma)
        {
            curr++;
            if (curr != end && curr->getTokenType() == coral66::lexer::TokenType::Ellipse)
            {
                ellipse = true;
                argumentList.push_back(*curr);
                curr++;
                break;
            }
            if (curr == end)
            {
                log(coral66::Errors::Parser::EXPECTED_N.args(*(curr - 1), *this, coral66::lexer::TokenType::Identifier,
                                                         *(curr - 1)));
                return {};
            }
            if (curr->getTokenType() != coral66::lexer::TokenType::Identifier)
            {
                log(coral66::Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(*curr, *this, coral66::lexer::TokenType::Identifier,
                                                                      *curr));
                return {};
            }
            auto string = curr->getValue();
            auto result =
                std::ranges::find_if(argumentList,
                             coral66::compose(coral66::bind_front(std::equal_to<>{}, string), &coral66::lexer::PPToken::getValue));
            if (result == argumentList.end())
            {
                argumentList.push_back(*(curr++));
                continue;
            }
            log(coral66::Errors::PP::REDEFINITION_OF_MACRO_PARAMETER_N.args(*curr, *this, *curr));
            log(coral66::Notes::PREVIOUSLY_DECLARED_HERE.args(*result, *this, *result));
            curr++;
        }
        if (curr == end)
        {
            log(coral66::Errors::Parser::EXPECTED_N.args(*(curr - 1), *this, coral66::lexer::TokenType::CloseParentheses,
                                                     *(curr - 1)));
            log(coral66::Notes::TO_MATCH_N_HERE.args(*openP, *this, *openP));
            return {};
        }
        if (curr->getTokenType() != coral66::lexer::TokenType::CloseParentheses)
        {
            log(coral66::Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(*curr, *this, coral66::lexer::TokenType::CloseParentheses,
                                                                  *curr));
            log(coral66::Notes::TO_MATCH_N_HERE.args(*openP, *this, *openP));
            return {};
        }
        curr++;
        return Macro{identifierPos, std::move(argumentList), ellipse, {curr, end}};
    }

    std::optional<bool> evaluateExpression(coral66::lexer::PPTokenIterator ifToken,
                                           std::span<const coral66::lexer::PPToken> tokens)
    {
        std::vector<coral66::lexer::PPToken> result;
        macroSubstitute({tokens.begin(), tokens.end()}, coral66::bind_front(&append<coral66::lexer::PPToken>, std::ref(result)),
                        true);
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
                    case coral66::lexer::CToken::Type::LongLong: iter->setType(coral66::lexer::CToken::Type::LongLong); break;
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
                CLD_ASSERT(!tokens.empty());
                log(coral66::Errors::PP::EXPECTED_AN_EXPRESSION_AFTER_IF.args(tokens, *this, tokens));
            }
            else
            {
                if (tokens.empty())
                {
                    log(coral66::Errors::PP::EXPECTED_AN_EXPRESSION_AFTER_ELIF.args(*ifToken, *this, *ifToken));
                }
                else
                {
                    log(coral66::Errors::PP::EXPECTED_AN_EXPRESSION_AFTER_ELIF_2.args(tokens, *this, tokens));
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
        coral66::Semantics::SemanticAnalysis analysis(*this, m_reporter, &errorsOccurred,
                                                  [this](std::string_view macro) {
                                                      return macro == "__FILE__" || macro == "__LINE__"
                                                             || m_defines.contains(macro);
                                                  });
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

    std::uint64_t getLineNumber(std::uint32_t const fileID, std::uint64_t const offset) const noexcept override
    {
        CORAL66_ASSERT(fileID < m_files.size());
        auto result = std::ranges::lower_bound(m_files[fileID].starts, offset);
        return std::distance(m_files[fileID].starts.begin(), result) + (*result == offset ? 1 : 0);
    }

    std::uint64_t getLineStartOffset(std::uint32_t const fileID, std::uint64_t const line) const noexcept override
    {
        CORAL66_ASSERT(fileID < m_files.size());
        CORAL66_ASSERT(line - 1 < m_files[fileID].starts.size());
        return m_files[fileID].starts[line - 1];
    }

    [[nodiscard]] std::uint64_t getLineEndOffset(std::uint32_t const fileID,
                                                 std::uint64_t const line) const noexcept override
    {
        CORAL66_ASSERT(fileID < m_files.size());
        CORAL66_ASSERT(line < m_files[fileID].starts.size());
        return m_files[fileID].starts[line] - 1;
    }

    std::span<const coral66::Source::File> getFiles() const noexcept override
    {
        return m_files;
    }

    std::span<const coral66::Source::PPRecord> getSubstitutions() const noexcept override
    {
        return m_substitutions;
    }

    std::span<const coral66::lexer::IntervalMap> getIntervalMaps() const noexcept override
    {
        return m_intervalMaps;
    }

public:
    Preprocessor(llvm::raw_ostream* report) noexcept : m_reporter(report) {}

    std::vector<coral66::lexer::PPToken>& getResult() noexcept
    {
        return m_result;
    }

    std::vector<coral66::Source::File>& getFiles() noexcept
    {
        return m_files;
    }

    std::vector<coral66::lexer::IntervalMap>& getIntervalMaps() noexcept
    {
        return m_intervalMaps;
    }

    const std::vector<coral66::Source::PPRecord>& getSubstitutionsVector() const
    {
        return m_substitutions;
    }

    void process(coral66::PPSourceObject&& sourceObject)
    {
        CORAL66_ASSERT(sourceObject.getFiles().size() == 1);
        m_files.push_back({std::move(sourceObject.getFiles()[0].path),
                           std::move(sourceObject.getFiles()[0].source),
                           std::move(sourceObject.getFiles()[0].starts),
                           sourceObject.getFiles()[0].systemHeader,
                           {},
                           {}});
        m_intervalMaps.push_back(std::move(sourceObject.getIntervalMap()[0]));

        coral66::pp::Context context(*this, m_reporter);
        const auto* begin = std::as_const(sourceObject).data().data();
        const auto tree = coral66::pp::parseMacro(
            begin, std::as_const(sourceObject).data().data() + std::as_const(sourceObject).data().size(), context);

        if (context.getErrorCount() != 0)
        {
            m_errorsOccurred = true;
            return;
        }

        visit(tree);
        m_fileTokens.push_back(std::move(sourceObject.data()));
    }
    void visit(const coral66::pp::TextBlock& text)
    {
        macroSubstitute({text.tokens.begin(), text.tokens.end()}, coral66::bind_front(&Preprocessor::pushLine, this));
    }

    void visit(const coral66::pp::File& file)
    {
        for (auto& iter : file.groups)
        {
            visit(iter);
        }
    }

    void visit(const coral66::pp::Group& group)
    {
        for (auto& iter : group.groupPart)
        {
            visit(iter);
        }
    }

    void visit(const coral66::pp::ControlLine& controlLine)
    {
        coral66::match(controlLine.variant, [this](auto&& value) { this->visit(value); });
    }

    void visit(const coral66::pp::DefineDirective& define)
    {
        if (define.tokens.empty() || define.tokens.front().getTokenType() != lexer::TokenType::Identifier)
        {
            return;
        }

        auto iter = define.tokens.begin();
        const auto* macroName = &(*iter);
        iter++;

        std::span<const coral66::lexer::PPToken> body(iter, define.tokens.end());
        m_defines[macroName->getValue()] = Macro{macroName, body};
    }

    void visit(const coral66::pp::GroupPart& groupPart)
    {
        coral66::match(groupPart, [this](auto&& value) { this->visit(value); });
    }

    static void visit(const coral66::lexer::PPTokenIterator&) {}

    bool errorsOccurred() const
    {
        return m_errorsOccurred;
    }
};

//
// Preprocessor entry point
//
PPSourceObject preprocess(PPSourceObject&& sourceObject, llvm::raw_ostream* reporter = &llvm::errs(),
                          bool* errorsOccurred = nullptr) noexcept;

template <class T>
std::string reconstruct(const T* begin, const T* end, const source_interface& sourceInterface) noexcept
{
    static_assert(std::is_base_of_v<lexer::TokenBase, T>);
    if (begin == end)
    {
        return {};
    }
    std::string result = lexer::normalizeSpelling(begin->getRepresentation(sourceInterface));
    const auto* prev = begin++;
    while (begin != end)
    {
        std::uint64_t newLines = 0;
        auto toMacroId0 = [&](std::uint32_t macroId, std::uint32_t fileId, std::uint64_t offset, bool isPrev)
        {
            auto tuple = std::make_tuple(macroId, fileId, offset);
            while (std::get<0>(tuple) != 0)
            {
                tuple = coral66::match(
                    sourceInterface.getSubstitutions()[std::get<0>(tuple)],
                    [](std::monostate) -> std::tuple<std::uint32_t, std::uint32_t, std::uint64_t>
                    { CORAL66_UNREACHABLE; },
                    [isPrev](const Source::Substitution& substitution)
                    {
                        if (isPrev && substitution.closeParentheses)
                        {
                            return std::make_tuple(substitution.closeParentheses->getMacroId(),
                                                   substitution.closeParentheses->getFileId(),
                                                   substitution.closeParentheses->getOffset());
                        }
                        return std::make_tuple(substitution.replacedIdentifier.getMacroId(),
                                               substitution.replacedIdentifier.getFileId(),
                                               substitution.replacedIdentifier.getOffset());
                    },
                    [](const Source::Stringification& stringification)
                    {
                        return std::make_tuple(stringification.replacedIdentifier.getMacroId(),
                                               stringification.replacedIdentifier.getFileId(),
                                               stringification.replacedIdentifier.getOffset());
                    },
                    [isPrev](const Source::TokenConcatenation& concat)
                    {
                        if (isPrev)
                        {
                            return std::make_tuple(concat.rightToken.getMacroId(), concat.rightToken.getFileId(),
                                                   concat.rightToken.getOffset());
                        }
                        return std::make_tuple(concat.leftToken.getMacroId(), concat.leftToken.getFileId(),
                                               concat.leftToken.getOffset());
                    });
            }
            return tuple;
        };
        auto beginValues = toMacroId0(begin->getMacroId(), begin->getFileId(), begin->getOffset(), false);
        auto prevValues = toMacroId0(prev->getMacroId(), prev->getFileId(), prev->getOffset(), true);
        // Both prev and begin are part of a function like macro then it can happen that prev will end up
        // pointing at the ) of the macro call while begin points at the identifier. For that case as
        // they are part of the same macro there cannot be a newline in between
        if (std::get<2>(beginValues) >= std::get<2>(prevValues))
        {
            newLines = std::min<std::size_t>(
                sourceInterface.getLineNumber(std::get<1>(beginValues), std::get<2>(beginValues))
                    - sourceInterface.getLineNumber(std::get<1>(prevValues), std::get<2>(prevValues)),
                1);
        }
        result.insert(result.size(), newLines, '\n');
        if (newLines == 0 && (begin->hasLeadingWhitespace()))
        {
            result += " " + lexer::normalizeSpelling(begin->getRepresentation(sourceInterface));
        }
        else
        {
            result += lexer::normalizeSpelling(begin->getRepresentation(sourceInterface));
        }
        prev = begin++;
    }
    return result;
}

} // namespace coral66::pp
