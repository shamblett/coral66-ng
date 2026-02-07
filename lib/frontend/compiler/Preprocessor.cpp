/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "Preprocessor.hpp"

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "Parser.hpp"
#include "PreprocessorParser.hpp"
#include "lib/frontend/common/error_messages.hpp"
#include "lib/support/ScopeExit.hpp"

namespace
{

template <class T>
std::vector<T>& append(std::vector<T>& lhs, std::vector<T>&& rhs)
{
    lhs.insert(lhs.end(), std::move_iterator(rhs.begin()), std::move_iterator(rhs.end()));
    return lhs;
}

class Preprocessor final : private coral66::PPSourceInterface
{
    llvm::raw_ostream* m_reporter;
    const coral66::LanguageOptions& m_languageOptions;
    const coral66::PP::Options& m_ppOptions;
    std::uint32_t m_macroID = 0;
    std::vector<coral66::Lexer::PPToken> m_result;
    std::vector<coral66::Source::PPRecord> m_substitutions{1};
    
    struct Macro
    {
        coral66::Lexer::PPTokenIterator identifierPos;
        std::span<const coral66::Lexer::PPToken> replacement;
    };
    std::unordered_map<std::string_view, Macro> m_defines;
    
    std::vector<coral66::Source::File> m_files;
    std::vector<std::vector<coral66::Lexer::PPToken>> m_fileTokens;
    std::vector<coral66::Lexer::IntervalMap> m_intervalMaps;
    bool m_errorsOccurred = false;

    void pushLine(std::vector<coral66::Lexer::PPToken>&& tokens)
    {
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
    // ReSharper disable once CppDFAUnreachableFunctionCall
    void macroSubstitute(std::vector<coral66::Lexer::PPToken>&& tokens, F lineOutput)
    {
        if (tokens.empty())
        {
            return;
        }
        std::vector<coral66::Lexer::PPToken> output;
        auto* start = tokens.data();
        
        for (auto* iter = tokens.data(); iter != tokens.data() + tokens.size();)
        {
            if (iter->getTokenType() == coral66::Lexer::TokenType::Newline)
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
            if (iter->getTokenType() != coral66::Lexer::TokenType::Identifier)
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
            m_substitutions.emplace_back(coral66::Source::Substitution{*result->second.identifierPos, *iter, {}, false});
            
            std::vector<coral66::Lexer::PPToken> replacement(result->second.replacement.begin(), result->second.replacement.end());
            for (auto& tok : replacement)
            {
                tok.setMacroId(i);
            }
            
            // Recursive substitution (simple approach)
            // Ideally we should guard against infinite recursion
            
            output.insert(output.end(), std::move_iterator(replacement.begin()), std::move_iterator(replacement.end()));
            
            iter++;
            start = iter;
        }
        output.insert(output.end(), std::move_iterator(start), std::move_iterator(tokens.data() + tokens.size()));
        lineOutput(std::move(output));
    }

    // ReSharper disable once CppDFAUnreachableFunctionCall
    std::optional<Macro> parseDefineDirective(const coral66::PP::DefineDirective& defineDirective)
    {
        if (defineDirective.tokens.size() != 2)
        {
            log(coral66::Errors::Parser::EXPECTED_N_AFTER_N.args(
                *defineDirective.defineToken, *this, coral66::Lexer::TokenType::Identifier, *defineDirective.defineToken));
            return {};
        }
        if (defineDirective.tokens[0].getTokenType() != coral66::Lexer::TokenType::Identifier)
        {
            log(coral66::Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(
                defineDirective.tokens[0], *this, coral66::Lexer::TokenType::Identifier, defineDirective.tokens[0]));
            return {};
        }
        if (defineDirective.tokens[1].getTokenType() != coral66::Lexer::TokenType::StringLiteral)
        {
            log(coral66::Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(
                defineDirective.tokens[1], *this, coral66::Lexer::TokenType::StringLiteral, defineDirective.tokens[1]));
            return {};
        }
        
        const auto* identifierPos = &defineDirective.tokens[0];
        std::string replacementText(defineDirective.tokens[1].getValue());
        
        bool errors = false;
        auto scratchPadPP = coral66::Lexer::tokenize(std::move(replacementText), &m_languageOptions, m_reporter, &errors, "<Macro Expansion>");
        if (errors)
        {
            m_errorsOccurred = true;
            return {};
        }
        
        m_fileTokens.push_back(std::move(scratchPadPP.data()));
        return Macro{identifierPos, {m_fileTokens.back().data(), m_fileTokens.back().size()}};
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

    [[nodiscard]] std::uint64_t getLineEndOffset(std::uint32_t const fileID, std::uint64_t const line) const noexcept override
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

    const coral66::LanguageOptions& getLanguageOptions() const noexcept override
    {
        return m_languageOptions;
    }

    std::span<const coral66::Lexer::IntervalMap> getIntervalMaps() const noexcept override
    {
        return m_intervalMaps;
    }

public:
    Preprocessor(llvm::raw_ostream* report, const coral66::PP::Options& ppOptions,
                 const coral66::LanguageOptions& languageOptions) noexcept
        : m_reporter(report), m_languageOptions(languageOptions), m_ppOptions(ppOptions)
    {
    }

    std::vector<coral66::Lexer::PPToken>& getResult() noexcept
    {
        return m_result;
    }

    std::vector<coral66::Source::File>& getFiles() noexcept
    {
        return m_files;
    }

    std::vector<coral66::Lexer::IntervalMap>& getIntervalMaps() noexcept
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
        
        coral66::PP::Context context(*this, m_reporter);
        const auto* begin = std::as_const(sourceObject).data().data();
        auto tree = coral66::PP::parseFile(
            begin, std::as_const(sourceObject).data().data() + std::as_const(sourceObject).data().size(), context);
        
        if (context.getErrorCount() != 0)
        {
            m_errorsOccurred = true;
            return;
        }
        
        visit(tree);
        m_fileTokens.push_back(std::move(sourceObject.data()));
    }

    void visit(const coral66::PP::File& file)
    {
        for (auto& iter : file.groups)
        {
            visit(iter);
        }
    }

    void visit(const coral66::PP::Group& group)
    {
        for (auto& iter : group.groupPart)
        {
            visit(iter);
        }
    }

    void visit(const coral66::PP::GroupPart& groupPart)
    {
        coral66::match(groupPart, [this](auto&& value) { this->visit(value); });
    }

    void visit(const coral66::PP::TextBlock& text)
    {
        macroSubstitute({text.tokens.begin(), text.tokens.end()}, coral66::bind_front(&Preprocessor::pushLine, this));
    }

    void visit(const coral66::PP::ControlLine& controlLine)
    {
        coral66::match(controlLine.variant, [this](auto&& value) { this->visit(value); });
    }

    void visit(const coral66::PP::DefineDirective& defineDirective)
    {
        if (auto macro = parseDefineDirective(defineDirective))
        {
            m_defines.emplace(macro->identifierPos->getValue(), *macro);
        }
    }
    
    // Unused visitors
    static void visit(const coral66::PP::IfSection&) {}
    static void visit(const coral66::PP::UnknownDirective&) {}
    static void visit(const coral66::PP::NonDirective&) {}
    
    // ControlLine variants
    static void visit(const coral66::PP::ControlLine::IncludeTag&) {}
    static void visit(const coral66::PP::ControlLine::LineTag&) {}
    static void visit(const coral66::PP::ControlLine::ErrorTag&) {}
    static void visit(const coral66::PP::ControlLine::PragmaTag&) {}
    static void visit(const coral66::Lexer::PPTokenIterator&) {} // undef

    bool errorsOccurred() const
    {
        return m_errorsOccurred;
    }
};
} // namespace

coral66::PPSourceObject coral66::PP::preprocess(coral66::PPSourceObject&& sourceObject, const Options& options,
                                        llvm::raw_ostream* reporter, bool* errorsOccurred) noexcept
{
    auto& languageOptions = sourceObject.getLanguageOptions();
    Preprocessor preprocessor(reporter, options, languageOptions);
    preprocessor.process(std::move(sourceObject));
    if (errorsOccurred)
    {
        *errorsOccurred = preprocessor.errorsOccurred();
    }
    return {std::move(preprocessor.getResult()), std::move(preprocessor.getFiles()), &languageOptions,
                          {preprocessor.getSubstitutionsVector().begin(), preprocessor.getSubstitutionsVector().end()},
                          {std::move(preprocessor.getIntervalMaps())}};
}
