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
#include "lib/frontend/lexer/lexer_support.hpp"
#include "lib/support/scope_exit.hpp"
#include "preprocessor_support.hpp"

void coral66::pp::Preprocessor::pushLine(std::vector<lexer::PPToken>&& tokens)
{
    if (m_visitingScratchPad)
    {
        return;
    }
    append(m_result, std::move(tokens));
}

bool coral66::pp::Preprocessor::log(const Message& message)
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
void coral66::pp::Preprocessor::process(PPSourceObject&& sourceObject)
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

std::string coral66::pp::Preprocessor::escapeString(std::string_view input)
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

std::pair<std::optional<coral66::pp::Preprocessor::Macro>, const coral66::lexer::PPToken*>
    coral66::pp::Preprocessor::parseDefineDirective(const DefineDirective& defineDirective)
{
    if (defineDirective.tokens.empty())
    {
        log(errors::pp::PP_EXPECTED_N_AFTER_N.args(*defineDirective.defineToken, *this, lexer::TokenType::Identifier,
                                                   *defineDirective.defineToken));
        preprocessor::DiagnosticCollector::addDiagnostic(errors::pp::PP_EXPECTED_N_AFTER_N);
        return {{}, nullptr};
    }
    if (defineDirective.tokens[0].getTokenType() != lexer::TokenType::Identifier)
    {
        log(errors::pp::PP_EXPECTED_N_INSTEAD_OF_N.args(defineDirective.tokens[0], *this, lexer::TokenType::Identifier,
                                                        defineDirective.tokens[0]));
        preprocessor::DiagnosticCollector::addDiagnostic(errors::pp::PP_EXPECTED_N_INSTEAD_OF_N);
        return {{}, nullptr};
    }
    if (defineDirective.tokens[0].getTokenType() == lexer::TokenType::Identifier)
    {
        if (lexer::isKeyword(defineDirective.tokens[0].getValue()))
        {
            log(errors::pp::KEYWORD_CANNOT_BE_A_MACRO_IDENTIFIER.args(defineDirective.tokens[0], *this, lexer::TokenType::Identifier,
                                                            defineDirective.tokens[0]));
            preprocessor::DiagnosticCollector::addDiagnostic(errors::pp::KEYWORD_CANNOT_BE_A_MACRO_IDENTIFIER);
            return {{}, nullptr};
        }
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
            preprocessor::DiagnosticCollector::addDiagnostic(
                errors::pp::WHITESPACE_REQUIRED_AFTER_OBJECT_MACRO_DEFINITION);
        }
        return {Macro{identifierPos, std::nullopt}, curr};
    }

    // We are a function like macro from here on
    const auto* openP = curr++;
    if (curr == end)
    {
        log(errors::pp::PP_EXPECTED_N.args(*openP, *this, lexer::TokenType::CloseParentheses, *openP));
        preprocessor::DiagnosticCollector::addDiagnostic(errors::pp::PP_EXPECTED_N);
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
        log(errors::pp::PP_EXPECTED_N_INSTEAD_OF_N.args(*curr, *this, lexer::TokenType::Identifier, *curr));
        preprocessor::DiagnosticCollector::addDiagnostic(errors::pp::PP_EXPECTED_N_INSTEAD_OF_N);
        return {{}, nullptr};
    }
    std::vector argumentList = {*curr};
    curr++;
    while (curr != end && curr->getTokenType() == lexer::TokenType::Comma)
    {
        curr++;
        if (curr == end)
        {
            log(errors::pp::PP_EXPECTED_N.args(*(curr - 1), *this, lexer::TokenType::Identifier, *(curr - 1)));
            preprocessor::DiagnosticCollector::addDiagnostic(errors::pp::PP_EXPECTED_N);
            return {{}, nullptr};
        }
        if (curr->getTokenType() != lexer::TokenType::Identifier)
        {
            log(errors::pp::PP_EXPECTED_N_INSTEAD_OF_N.args(*curr, *this, lexer::TokenType::Identifier, *curr));
            preprocessor::DiagnosticCollector::addDiagnostic(errors::pp::PP_EXPECTED_N_INSTEAD_OF_N);
            return {{}, nullptr};
        }
        auto string = curr->getValue();
        auto result = std::ranges::find_if(
            argumentList, compose(coral66::bind_front(std::equal_to<>{}, string), &lexer::PPToken::getValue));
        if (result == argumentList.end())
        {
            argumentList.push_back(*(curr++));
            continue;
        }
        log(errors::pp::REDEFINITION_OF_MACRO_PARAMETER_N.args(*curr, *this, *curr));
        preprocessor::DiagnosticCollector::addDiagnostic(errors::pp::REDEFINITION_OF_MACRO_PARAMETER_N);
        log(notes::PREVIOUSLY_DECLARED_HERE.args(*result, *this, *result));
        curr++;
    }
    if (curr == end)
    {
        log(errors::pp::PP_EXPECTED_N.args(*(curr - 1), *this, lexer::TokenType::CloseParentheses, *(curr - 1)));
        preprocessor::DiagnosticCollector::addDiagnostic(errors::pp::PP_EXPECTED_N);
        log(notes::TO_MATCH_N_HERE.args(*openP, *this, *openP));
        return {{}, nullptr};
    }
    if (curr->getTokenType() != lexer::TokenType::CloseParentheses)
    {
        log(errors::pp::PP_EXPECTED_N_INSTEAD_OF_N.args(*curr, *this, lexer::TokenType::CloseParentheses, *curr));
        preprocessor::DiagnosticCollector::addDiagnostic(errors::pp::PP_EXPECTED_N_INSTEAD_OF_N);
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
    auto cTokens =
        coral66::lexer::toCTokens(result.data(), result.data() + result.size(), *this, m_reporter, &errorsOccurred);
    if (errorsOccurred)
    {
        m_errorsOccurred = true;
        return {};
    }
    for (auto iter = cTokens.begin(); iter != cTokens.end(); ++iter)
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
                if (iter + 1 == cTokens.end())
                {
                    continue;
                }
                if ((iter + 1)->getTokenType() == lexer::TokenType::Identifier)
                {
                    ++iter;
                }
                else if ((iter + 1)->getTokenType() == lexer::TokenType::OpenParentheses)
                {
                    if (iter + 2 != cTokens.end() && (iter + 2)->getTokenType() == lexer::TokenType::Identifier)
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
    return {};
}

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

void coral66::pp::Preprocessor::visit(const TextBlock& text)
{
    macroSubstitute({text.tokens.begin(), text.tokens.end()}, coral66::bind_front(&Preprocessor::pushLine, this));
}

void coral66::pp::Preprocessor::visit(const File& file)
{
    for (auto& iter : file.groups)
    {
        visit(iter);
    }
}

void coral66::pp::Preprocessor::visit(const Group& group)
{
    for (auto& iter : group.groupPart)
    {
        visit(iter);
    }
}

void coral66::pp::Preprocessor::visit(const ControlLine& controlLine)
{
    match(controlLine.variant, [this](auto&& value) { this->visit(value); });
}

void coral66::pp::Preprocessor::visit(const DefineDirective& define)
{
    auto parse_res = parseDefineDirective(define);
    if (!parse_res.first)
        return;
    auto optMacro = *parse_res.first;
    auto iterPtr = parse_res.second;

    auto iter = define.tokens.begin() + (iterPtr - define.tokens.data());

    std::vector<lexer::PPToken> replacementTokens;
    if (iter != define.tokens.end() && iter->getTokenType() == lexer::TokenType::StringLiteral)
    {
        auto stringVal = iter->getValue();
        std::string innerString(stringVal);
        if (innerString.size() >= 2 && innerString.front() == '"' && innerString.back() == '"')
        {
            innerString = innerString.substr(1, innerString.size() - 2);
        }

        bool errors = false;
        auto innerSource = coral66::lexer::tokenize(innerString, m_reporter, &errors, "Macro");
        if (errors)
        {
            m_errorsOccurred = true;
        }

        auto fileOffset = m_files.size();
        for (auto& file : innerSource.getFiles())
        {
            m_files.push_back(std::move(file));
        }
        for (auto& map : innerSource.getIntervalMap())
        {
            m_intervalMaps.push_back(std::move(map));
        }

        replacementTokens = std::move(innerSource.data());
        for (auto& token : replacementTokens)
        {
            token.setFileId(token.getFileId() + fileOffset);
        }

        ++iter;
        replacementTokens.insert(replacementTokens.end(), iter, define.tokens.end());
    }
    else if (iter != define.tokens.end() && iter->getTokenType() == lexer::TokenType::Miscellaneous)
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

void coral66::pp::Preprocessor::visit(const GroupPart& groupPart)
{
    match(groupPart, [this](auto&& value) { this->visit(value); });
}

void coral66::pp::Preprocessor::visit(const lexer::PPTokenIterator& name)
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

coral66::PPSourceObject coral66::pp::preprocess(PPSourceObject&& sourceObject, llvm::raw_ostream* reporter,
                                                bool* errorsOccurred) noexcept
{
    preprocessor::DiagnosticCollector::clearDiagnostics();
    Preprocessor preprocessor(reporter);
    preprocessor.process(std::move(sourceObject));
    if (errorsOccurred)
    {
        *errorsOccurred = preprocessor.errorsOccurred();
    }
    auto result =
        PPSourceObject(std::move(preprocessor.getResult()), std::move(preprocessor.getFiles()),
                       {preprocessor.getSubstitutionsVector().begin(), preprocessor.getSubstitutionsVector().end()},
                       {std::move(preprocessor.getIntervalMaps())});
    result.addDiagnostics(preprocessor::DiagnosticCollector::getDiagnostics());
    return result;
}
