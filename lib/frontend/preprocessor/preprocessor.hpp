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

class Preprocessor final : PPSourceInterface
{
    llvm::raw_ostream* m_reporter;
    std::uint32_t m_macroID = 0;
    std::vector<lexer::PPToken> m_result;
    std::vector<Source::PPRecord> m_substitutions{1};

    struct Macro
    {
        lexer::PPTokenIterator identifierPos{};
        std::optional<std::vector<lexer::PPToken>> argumentList;
        std::span<const lexer::PPToken> replacement;
    };
    std::unordered_map<std::string_view, std::vector<Macro>> m_defines;

    std::vector<std::unordered_set<lexer::PPTokenIterator>> m_disabledMacros{1};
    std::vector<Source::File> m_files;
    std::vector<std::vector<lexer::PPToken>> m_fileTokens;
    std::vector<lexer::IntervalMap> m_intervalMaps;
    bool m_errorsOccurred = false;
    bool m_visitingScratchPad = false;

    void pushLine(std::vector<lexer::PPToken>&& tokens)
    {
        if (m_visitingScratchPad)
        {
            return;
        }
        append(m_result, std::move(tokens));
    }

    // ReSharper disable once CppDFAUnreachableFunctionCall
    bool log(const Message& message)
    {
        if (m_reporter)
        {
            *m_reporter << message;
        }
        if (message.getSeverity() == Severity::Error)
        {
            m_errorsOccurred = true;
        }
        return message.getSeverity() != Severity::None;
    }

    template <class F>
    void macroSubstitute(std::vector<lexer::PPToken>&& tokens, F lineOutput)
    {
        if (tokens.empty())
        {
            return;
        }
        std::vector<lexer::PPToken> output;

        for (long idx = 0; idx < tokens.size();)
        {
            if (tokens[idx].getTokenType() == coral66::lexer::TokenType::Newline)
            {
                idx++;
                if (idx == tokens.size())
                {
                    break;
                }
                lineOutput(std::move(output));
                output.clear();
                continue;
            }
            if (tokens[idx].getTokenType() != coral66::lexer::TokenType::Identifier)
            {
                output.push_back(std::move(tokens[idx]));
                idx++;
                continue;
            }

            auto name = tokens[idx].getValue();
            auto result = m_defines.find(name);
            if (result == m_defines.end() || result->second.empty())
            {
                output.push_back(std::move(tokens[idx]));
                idx++;
                continue;
            }

            // Expand Macro
            const auto i = ++m_macroID;
            m_substitutions.emplace_back(
                Source::Substitution{*result->second.back().identifierPos, tokens[idx], {}, false});

            std::vector replacement(result->second.back().replacement.begin(), result->second.back().replacement.end());
            for (auto& tok : replacement)
            {
                tok.setMacroId(i);
            }

            long end_idx = idx + 1;

            if (result->second.back().argumentList.has_value())
            {
                auto params = result->second.back().argumentList.value();
                auto param_count = params.size();

                long arg_idx = idx + 1;
                while (arg_idx < tokens.size() && tokens[arg_idx].getTokenType() == coral66::lexer::TokenType::Newline)
                    arg_idx++;

                if (arg_idx == tokens.size()
                    || tokens[arg_idx].getTokenType() != coral66::lexer::TokenType::OpenParentheses)
                {
                    output.push_back(std::move(tokens[idx]));
                    idx++;
                    continue; // Skip expansion, wait for parens or treat as not a macro invocation
                }

                arg_idx++;
                std::vector<std::vector<lexer::PPToken>> actual_args;
                std::vector<lexer::PPToken> current_arg;
                int paren_depth = 0;
                int square_depth = 0;

                while (arg_idx < tokens.size())
                {
                    if (tokens[arg_idx].getTokenType() == coral66::lexer::TokenType::OpenParentheses)
                        paren_depth++;
                    else if (tokens[arg_idx].getTokenType() == coral66::lexer::TokenType::CloseParentheses)
                    {
                        if (paren_depth == 0)
                            break;
                        paren_depth--;
                    }
                    else if (tokens[arg_idx].getTokenType() == coral66::lexer::TokenType::OpenSquareBracket)
                        square_depth++;
                    else if (tokens[arg_idx].getTokenType() == coral66::lexer::TokenType::CloseSquareBracket)
                        square_depth--;
                    else if (tokens[arg_idx].getTokenType() == coral66::lexer::TokenType::Comma && paren_depth == 0
                             && square_depth == 0)
                    {
                        actual_args.push_back(current_arg);
                        current_arg.clear();
                        arg_idx++;
                        continue;
                    }

                    current_arg.push_back(tokens[arg_idx]);
                    arg_idx++;
                }
                actual_args.push_back(current_arg);
                if (arg_idx < tokens.size()
                    && tokens[arg_idx].getTokenType() == coral66::lexer::TokenType::CloseParentheses)
                    arg_idx++;

                if (actual_args.size() != param_count
                    && !(param_count == 0 && actual_args.size() == 1 && actual_args[0].empty()))
                {
                    output.push_back(std::move(tokens[idx]));
                    idx++;
                    continue;
                }

                std::vector<lexer::PPToken> new_replacement;
                for (auto& tok : replacement)
                {
                    if (tok.getTokenType() == coral66::lexer::TokenType::Identifier)
                    {
                        auto tok_val = tok.getValue();
                        bool substituted = false;
                        for (size_t p = 0; p < param_count; ++p)
                        {
                            if (tok_val == params[p].getValue())
                            {
                                new_replacement.insert(new_replacement.end(), actual_args[p].begin(),
                                                       actual_args[p].end());
                                substituted = true;
                                break;
                            }
                        }
                        if (!substituted)
                            new_replacement.push_back(tok);
                    }
                    else
                    {
                        new_replacement.push_back(tok);
                    }
                }
                replacement = new_replacement;
                end_idx = arg_idx;
            }

            // Recursive substitution: Replace the macro invocation with its replacement tokens
            tokens.erase(tokens.begin() + idx, tokens.begin() + end_idx);
            tokens.insert(tokens.begin() + idx, std::move_iterator(replacement.begin()),
                          std::move_iterator(replacement.end()));
            // DO NOT idx++; Let the loop process the newly inserted tokens
        }

        // Append any remaining trailing newlines to be consistent with original behavior?
        // Actually, the original did output.insert(..., start, size()) which would include the trailing newline if any.
        // Let's just output whatever is left.
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

    std::pair<std::optional<Macro>, const lexer::PPToken*> parseDefineDirective(const DefineDirective& defineDirective);

    std::optional<bool> evaluateExpression(lexer::PPTokenIterator ifToken, std::span<const lexer::PPToken> tokens);

    std::uint64_t getLineNumber(std::uint32_t fileID, std::uint64_t offset) const noexcept override;

    std::uint64_t getLineStartOffset(std::uint32_t fileID, std::uint64_t line) const noexcept override;

    [[nodiscard]] std::uint64_t getLineEndOffset(std::uint32_t fileID, std::uint64_t line) const noexcept override;

    std::span<const Source::File> getFiles() const noexcept override
    {
        return m_files;
    }

    std::span<const Source::PPRecord> getSubstitutions() const noexcept override
    {
        return m_substitutions;
    }

    std::span<const lexer::IntervalMap> getIntervalMaps() const noexcept override
    {
        return m_intervalMaps;
    }

public:
    explicit Preprocessor(llvm::raw_ostream* report) noexcept : m_reporter(report) {}

    std::vector<lexer::PPToken>& getResult() noexcept
    {
        return m_result;
    }

    std::vector<Source::File>& getFiles() noexcept
    {
        return m_files;
    }

    std::vector<lexer::IntervalMap>& getIntervalMaps() noexcept
    {
        return m_intervalMaps;
    }

    const std::vector<Source::PPRecord>& getSubstitutionsVector() const
    {
        return m_substitutions;
    }

    void process(PPSourceObject&& sourceObject)
    {
        CORAL66_ASSERT(sourceObject.getFiles().size() == 1);
        m_files.push_back({std::move(sourceObject.getFiles()[0].path),
                           std::move(sourceObject.getFiles()[0].source),
                           std::move(sourceObject.getFiles()[0].starts),
                           sourceObject.getFiles()[0].systemHeader,
                           {},
                           {}});
        m_intervalMaps.push_back(std::move(sourceObject.getIntervalMap()[0]));

        Context context(*this, m_reporter);
        const auto* begin = std::as_const(sourceObject).data().data();
        const auto tree = parseMacro(
            begin, std::as_const(sourceObject).data().data() + std::as_const(sourceObject).data().size(), context);

        if (context.getErrorCount() != 0)
        {
            m_errorsOccurred = true;
            return;
        }

        visit(tree);
        m_fileTokens.push_back(std::move(sourceObject.data()));
    }
    void visit(const TextBlock& text)
    {
        macroSubstitute({text.tokens.begin(), text.tokens.end()}, coral66::bind_front(&Preprocessor::pushLine, this));
    }

    void visit(const File& file)
    {
        for (auto& iter : file.groups)
        {
            visit(iter);
        }
    }

    void visit(const Group& group)
    {
        for (auto& iter : group.groupPart)
        {
            visit(iter);
        }
    }

    void visit(const ControlLine& controlLine)
    {
        match(controlLine.variant, [this](auto&& value) { this->visit(value); });
    }

    void visit(const DefineDirective& define)
    {
        auto parse_res = parseDefineDirective(define);
        if (!parse_res.first)
            return;
        auto optMacro = *parse_res.first;
        auto iterPtr = parse_res.second;

        auto iter = define.tokens.begin() + (iterPtr - define.tokens.data());

        std::vector<lexer::PPToken> replacementTokens;
        if (iter != define.tokens.end() && iter->getTokenType() == lexer::TokenType::Miscellaneous)
        {
            ++iter;
            auto last = define.tokens.end();
            if (last - iter >= 1 && (last - 1)->getTokenType() == lexer::TokenType::Miscellaneous)
            {
                replacementTokens.assign(iter, last - 1);
            }
            else
            {
                replacementTokens.assign(iter, last);
            }
        }
        else
        {
            replacementTokens.assign(iter, define.tokens.end());
        }

        m_fileTokens.push_back(std::move(replacementTokens));
        std::span<const lexer::PPToken> replacementSpan(m_fileTokens.back().data(), m_fileTokens.back().size());

        optMacro.replacement = replacementSpan;
        m_defines[optMacro.identifierPos->getValue()].push_back(optMacro);

        if (define.defineToken != lexer::PPTokenIterator{})
        {
            std::vector<lexer::PPToken> outputTokens;
            outputTokens.push_back(*define.defineToken);
            outputTokens.insert(outputTokens.end(), define.tokens.begin(), define.tokens.end());
            outputTokens.push_back(*define.tokens.end()); // The semicolon
            pushLine(std::move(outputTokens));
        }
    }

    void visit(const GroupPart& groupPart)
    {
        match(groupPart, [this](auto&& value) { this->visit(value); });
    }

    void visit(const lexer::PPTokenIterator& name)
    {
        if (name == lexer::PPTokenIterator{})
            return;

        std::vector<lexer::PPToken> outputTokens;
        outputTokens.push_back(*(name - 1)); // DELETE keyword
        outputTokens.push_back(*name);       // Macroname
        outputTokens.push_back(*(name + 1)); // Semicolon
        pushLine(std::move(outputTokens));

        auto it = m_defines.find(name->getValue());
        if (it != m_defines.end() && !it->second.empty())
        {
            it->second.pop_back();
            if (it->second.empty())
            {
                m_defines.erase(it);
            }
        }
    }

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
