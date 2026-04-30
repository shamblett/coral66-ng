#include "lib/frontend/common/color_string_ostream.hpp"
/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/WithColor.h>

#include <algorithm>
#include <array>
#include <map>

#include "diagnostic.hpp"
#include "source_object.hpp"

namespace
{
using PointLocation = coral66::diag::PointLocation;

std::pair<PointLocation, PointLocation> toMacroId0Range(std::pair<PointLocation, PointLocation> location,
                                                        const coral66::source_interface& sourceInterface)
{
    while (location.first.macroId != 0)
    {
        location.first = coral66::match(
            sourceInterface.getSubstitutions()[location.first.macroId],
            [](std::monostate) -> PointLocation { CORAL66_UNREACHABLE; },
            [](const coral66::Source::Substitution& substitution) -> PointLocation
            {
                return {substitution.replacedIdentifier.getOffset(), substitution.replacedIdentifier.getFileId(),
                        substitution.replacedIdentifier.getMacroId()};
            },
            [](const coral66::Source::Stringification& substitution) -> PointLocation
            {
                return {substitution.replacedIdentifier.getOffset(), substitution.replacedIdentifier.getFileId(),
                        substitution.replacedIdentifier.getMacroId()};
            },
            [](const coral66::Source::TokenConcatenation& substitution) -> PointLocation
            {
                // I think it doesn't matter which we return here? Let's do left for now
                return {substitution.leftToken.getOffset(), substitution.leftToken.getFileId(),
                        substitution.leftToken.getMacroId()};
            });
    }
    while (location.second.macroId != 0)
    {
        location.second = coral66::match(
            sourceInterface.getSubstitutions()[location.second.macroId],
            [](std::monostate) -> PointLocation { CORAL66_UNREACHABLE; },
            [](const coral66::Source::Substitution& substitution) -> PointLocation
            {
                return {substitution.replacedIdentifier.getOffset() + substitution.replacedIdentifier.getLength(),
                        substitution.replacedIdentifier.getFileId(), substitution.replacedIdentifier.getMacroId()};
            },
            [](const coral66::Source::Stringification& substitution) -> PointLocation
            {
                return {substitution.replacedIdentifier.getOffset() + substitution.replacedIdentifier.getLength(),
                        substitution.replacedIdentifier.getFileId(), substitution.replacedIdentifier.getMacroId()};
            },
            [](const coral66::Source::TokenConcatenation& substitution) -> PointLocation
            {
                // I think it doesn't matter which we return here? Let's do left for now
                return {substitution.rightToken.getOffset() + substitution.rightToken.getLength(),
                        substitution.rightToken.getFileId(), substitution.rightToken.getMacroId()};
            });
    }
    return location;
}

std::pair<std::uint64_t, std::uint64_t> getLineCol(const PointLocation& pointLocation,
                                                   const coral66::source_interface& sourceInterface)
{
    const auto line = sourceInterface.getLineNumber(pointLocation.fileId, pointLocation.offset);
    const auto column = pointLocation.offset - sourceInterface.getLineStartOffset(pointLocation.fileId, line) + 1;
    return {line, column};
}

struct ModifierOfLine
{
    struct Underline
    {
        char character;
        bool continuous;
    };
    struct InsertAfter
    {
        std::string_view text;
    };
    struct Annotate
    {
        std::string_view text;
        std::uint64_t pos;
    };
    std::pair<std::uint64_t, std::uint64_t> fromToInLine;
    using variant = std::variant<Underline, InsertAfter, Annotate>;
    variant value;
};

void printLine(llvm::raw_ostream& ss, std::uint64_t const line, std::uint64_t const lineStartOffset,
               const std::string_view text, const std::uint64_t width, const std::span<ModifierOfLine> modifiersOfLine)
{
    ss << llvm::format_decimal(line, width) << " | ";
    ss.write(text.data(), text.size()) << '\n';
    if (modifiersOfLine.empty())
    {
        return;
    }
    const auto widthOfText = coral66::unsafeColumnWidth(text);
    std::vector<std::string> canvas;
    std::string extraAtEnd;
    for (auto& iter : modifiersOfLine)
    {
        coral66::match(
            iter.value,
            [&](const ModifierOfLine::Underline& underline)
            {
                auto indent = coral66::unsafeColumnWidth(text.substr(0, iter.fromToInLine.first - lineStartOffset));
                canvas.resize(std::max<std::size_t>(1, canvas.size()), std::string(widthOfText, ' '));

                if (iter.fromToInLine.first == iter.fromToInLine.second && text.size() == iter.fromToInLine.first)
                {
                    // Special case when we are pointing at a newline character
                    canvas[0].resize(std::max<std::size_t>(indent + 1, canvas[0].size()), ' ');
                    canvas[0][indent] = underline.character;
                    return;
                }

                if (underline.continuous)
                {
                    const auto sizeOfTextAbove = coral66::unsafeColumnWidth(text.substr(
                        iter.fromToInLine.first - lineStartOffset, iter.fromToInLine.second - iter.fromToInLine.first));
                    std::fill_n(canvas[0].begin() + indent, sizeOfTextAbove, underline.character);
                    return;
                }

                const auto textAbove = text.substr(iter.fromToInLine.first - lineStartOffset,
                                                   iter.fromToInLine.second - iter.fromToInLine.first);
                for (const auto* ptr = textAbove.data(); ptr != textAbove.data() + textAbove.size();)
                {
                    // Skip all whitespace first
                    while (ptr != textAbove.data() + textAbove.size())
                    {
                        llvm::UTF32 utf32;
                        llvm::UTF32* targetStart = &utf32;
                        const auto* before = ptr;
                        auto result = llvm::ConvertUTF8toUTF32(
                            reinterpret_cast<const llvm::UTF8**>(&ptr),
                            reinterpret_cast<const llvm::UTF8*>(textAbove.data() + textAbove.size()), &targetStart, targetStart + 1,
                            llvm::strictConversion);
                        (void)result;
                        CORAL66_ASSERT(result == llvm::conversionOK || result == llvm::targetExhausted);
                        if (!coral66::isWhitespace(utf32))
                        {
                            ptr = before;
                            break;
                        }
                    }

                    // Then find the longest chain of pure non whitespace
                    const auto* start = ptr;
                    while (start != textAbove.data() + textAbove.size())
                    {
                        llvm::UTF32 utf32;
                        llvm::UTF32* targetStart = &utf32;
                        const auto* before = start;
                        auto result = llvm::ConvertUTF8toUTF32(
                            reinterpret_cast<const llvm::UTF8**>(&start),
                            reinterpret_cast<const llvm::UTF8*>(textAbove.data() + textAbove.size()), &targetStart, targetStart + 1,
                            llvm::strictConversion);
                        (void)result;
                        CORAL66_ASSERT(result == llvm::conversionOK || result == llvm::targetExhausted);
                        if (coral66::isWhitespace(utf32))
                        {
                            start = before;
                            break;
                        }
                    }
                    const auto size = coral66::unsafeColumnWidth({ptr, static_cast<std::size_t>(start - ptr)});
                    indent = coral66::unsafeColumnWidth(
                        text.substr(0, iter.fromToInLine.first - lineStartOffset + ptr - textAbove.data()));
                    std::fill_n(canvas[0].begin() + indent, size, underline.character);
                    ptr = start;
                }
            },
            [&](const ModifierOfLine::InsertAfter& insertAfter)
            {
                if (iter.fromToInLine.first == iter.fromToInLine.second)
                {
                    // We are inserting after a Newline
                    extraAtEnd = std::string(width, ' ') + " > " + coral66::to_string(insertAfter.text);
                    return;
                }
                const auto indent =
                    coral66::unsafeColumnWidth(text.substr(0, iter.fromToInLine.second - lineStartOffset));
                canvas.resize(std::max<std::size_t>(1, canvas.size()), std::string(widthOfText, ' '));
                canvas[0].resize(std::max<std::size_t>(canvas[0].size(), indent + 1));
                canvas[0][indent] = '^';
                if (insertAfter.text.empty())
                {
                    return;
                }
                if (canvas.size() == 1)
                {
                    canvas.resize(2, std::string(widthOfText, ' '));
                }
                else
                {
                    // Check if we need to expand height. To do that check if all chars where we'd place the
                    // text + 1 are all space
                    const bool allAreSpace = std::all_of(
                        canvas.back().begin() + indent,
                        canvas.back().begin()
                            + std::min<std::size_t>(indent + insertAfter.text.size() + 1, canvas.back().size()),
                        coral66::bind_front(std::equal_to{}, ' '));
                    if (!allAreSpace)
                    {
                        canvas.resize(canvas.size() + 1, std::string(widthOfText, ' '));
                    }
                }
                for (std::size_t i = 1; i < canvas.size() - 1; i++)
                {
                    canvas[i].resize(std::max<std::size_t>(canvas[i].size(), indent + 1), ' ');
                    canvas[i][indent] = '|';
                }
                canvas.back().resize(std::max(canvas.back().size(), indent + insertAfter.text.size()), ' ');
                std::ranges::copy(insertAfter.text, canvas.back().begin() + indent);
            },
            [&](const ModifierOfLine::Annotate& annotate)
            {
                auto indent = coral66::unsafeColumnWidth(text.substr(0, iter.fromToInLine.first - lineStartOffset));
                canvas.resize(std::max<std::size_t>(1, canvas.size()), std::string(widthOfText, ' '));
                const auto sizeOfTextAbove = coral66::unsafeColumnWidth(text.substr(
                    iter.fromToInLine.first - lineStartOffset, iter.fromToInLine.second - iter.fromToInLine.first));
                std::fill_n(canvas[0].begin() + indent, sizeOfTextAbove, sizeOfTextAbove == 1 ? '^' : '~');
                if (iter.fromToInLine.first > annotate.pos || annotate.pos > iter.fromToInLine.second)
                {
                    return;
                }
                if (canvas.size() < 3)
                {
                    canvas.resize(3, std::string(widthOfText, ' '));
                }
                indent = coral66::unsafeColumnWidth(text.substr(0, annotate.pos - lineStartOffset));
                // Check if we need to expand height
                bool allAreSpace =
                    std::all_of(canvas.back().begin() + indent,
                                canvas.back().begin()
                                    + std::min<std::size_t>(indent + annotate.text.size() + 1, canvas.back().size()),
                                coral66::bind_front(std::equal_to{}, ' '));
                if (!allAreSpace)
                {
                    canvas.resize(canvas.size() + 1, std::string(widthOfText, ' '));
                }
                for (std::size_t i = 1; i < canvas.size() - 1; i++)
                {
                    canvas[i].resize(std::max<std::size_t>(canvas[i].size(), indent + 1), ' ');
                    canvas[i][indent] = '|';
                }
                canvas.back().resize(std::max(canvas.back().size(), indent + annotate.text.size()), ' ');
                std::ranges::copy(annotate.text, canvas.back().begin() + indent);
            });
    }
    for (auto& iter : canvas)
    {
        ss.indent(width) << " | " << iter << '\n';
    }
    if (!extraAtEnd.empty())
    {
        ss << extraAtEnd << '\n';
    }
}

} // namespace

auto coral66::diag::after(const lexer::TokenBase& token) -> std::tuple<const lexer::TokenBase&, std::uint64_t>
{
    return std::make_tuple(std::cref(token), token.getOffset() + token.getLength());
}

coral66::Message coral66::detail::Diagnostic::DiagnosticBase::print(
    std::pair<diag::PointLocation, diag::PointLocation> location, std::string_view message,
    std::span<Argument> arguments, std::span<const Modifiers> modifiers, const source_interface& sourceInterface) const
{
    if (m_severity == Severity::Warning)
    {
        return {};
    }
    std::string result;
    // TODO: Right now the colour is useless. Just putting it here to know what the colour when and were.
    // Will have to create a segment map or so in coral66::Message that stores the colour and boldness information
    // and then emits it correctly in it's operator<<
    ColorStringOstream ss(result);

    {
        auto file = sourceInterface.getFiles()[location.first.fileId].includedBy;
        std::vector<std::pair<std::uint32_t, std::uint64_t>> includeTrace;
        while (file)
        {
            includeTrace.push_back(*file);
            file = sourceInterface.getFiles()[file->first].includedBy;
        }
        std::ranges::reverse(includeTrace);
        for (auto& iter : includeTrace)
        {
            auto line = sourceInterface.getLineNumber(iter.first, iter.second);
            const auto& range = sourceInterface.getFiles()[iter.first].lineAndFileMapping;
            auto map =
                llvm::upper_bound(range, line, [](auto line, const auto& tuple) { return line < std::get<0>(tuple); });
            if (!range.empty() && map == range.end())
            {
                --map;
            }
            if (map == range.end())
            {
                // The element was not found. That means that any #lines appeared after this line or that none exist
                llvm::WithColor(ss, llvm::raw_string_ostream::WHITE, true)
                    << "In file included from " << sourceInterface.getFiles()[iter.first].path << ':' << line << ':';
            }
            else
            {
                const auto path = std::get<1>(*map).value_or(sourceInterface.getFiles()[iter.first].path);
                const auto printedLine = std::get<2>(*map) + line - std::get<0>(*map);
                llvm::WithColor(ss, llvm::raw_string_ostream::WHITE, true)
                    << "In file included from " << path << ':' << printedLine << ':';
            }
            ss << '\n';
        }
    }

    location = toMacroId0Range(location, sourceInterface);
    {
        const auto [line, column] = getLineCol(location.first, sourceInterface);
        const auto& range = sourceInterface.getFiles()[location.first.fileId].lineAndFileMapping;
        auto iter =
            llvm::upper_bound(range, line, [](auto line, const auto& tuple) { return line < std::get<0>(tuple); });
        if (!range.empty() && iter == range.end())
        {
            --iter;
        }
        if (iter == range.end())
        {
            // The element was not found. That means that any #lines appeared after this line or that none exist
            llvm::WithColor(ss, llvm::raw_string_ostream::WHITE, true)
                << sourceInterface.getFiles()[location.first.fileId].path << ':' << line << ':' << column << ": ";
        }
        else
        {
            const auto path = std::get<1>(*iter).value_or(sourceInterface.getFiles()[location.first.fileId].path);
            const auto printedLine = std::get<2>(*iter) + line - std::get<0>(*iter);
            llvm::WithColor(ss, llvm::raw_string_ostream::WHITE, true)
                << path << ':' << printedLine << ':' << column << ": ";
        }
    }

    evaluateFormatsInMessage(message, arguments, ss);

    // The second argument doesn't use getLineCol for location.second because the std::pair location
    // is an open range aka [location.first,location.second). Therefore, to get the actual last line
    // that needs to be printed we need to do offset - 1 to get the closed range end
    std::map<std::uint32_t, std::pair<std::uint64_t, std::uint64_t>> fileToMaxMinLine = {
        {location.first.fileId,
         {getLineCol(location.first, sourceInterface).first,
          sourceInterface.getLineNumber(location.second.fileId, location.second.offset - 1)}}};
    // Using a map cause I am too lazy to get a std::hash function for std::pair now. Future TODO
    std::map<std::pair<std::uint32_t, std::uint64_t>, std::vector<ModifierOfLine>> fileAndLineToModifiers;
    for (auto& iter : modifiers)
    {
        const auto index = coral66::match(
            iter, [](const DiagnosticBase::Underline& underline) { return underline.index; },
            [](const DiagnosticBase::InsertAfter& insertAfter) { return insertAfter.index; },
            [](const DiagnosticBase::Annotate& annotate) { return annotate.index; });
        CORAL66_ASSERT(arguments[index].range);
        arguments[index].range = toMacroId0Range(*arguments[index].range, sourceInterface);
        auto startLine = getLineCol(arguments[index].range->first, sourceInterface).first;
        auto endLine = getLineCol(arguments[index].range->second, sourceInterface).first;
        auto fileId = arguments[index].range->first.fileId;
        if (auto found = fileToMaxMinLine.find(fileId); found != fileToMaxMinLine.end())
        {
            found->second = {std::min(found->second.first, startLine), std::max(found->second.second, endLine)};
        }
        else
        {
            // TODO: Might need to split up fileID here
            fileToMaxMinLine.insert({fileId, {startLine, endLine}});
        }
        for (auto i = startLine; i <= endLine; i++)
        {
            // TODO: Might need to split up fileID here
            auto& vector = fileAndLineToModifiers[{fileId, i}];
            auto startOffset = sourceInterface.getLineStartOffset(fileId, i);
            auto endOffset = sourceInterface.getLineEndOffset(fileId, i);
            if (arguments[index].range->first.offset == startOffset - 1
                && arguments[index].range->second.offset == startOffset)
            {
                // This case only occurs when we are pointing at a newline
                // into the second line
                // We want to handle this specially for InsertAfter
                CORAL66_ASSERT(!std::holds_alternative<Annotate>(iter));
                continue;
            }
            vector.push_back(
                {{std::max(arguments[index].range->first.offset, startOffset),
                  std::min(arguments[index].range->second.offset, endOffset)},
                 coral66::match(
                     iter, [](const DiagnosticBase::Underline& underline) -> ModifierOfLine::variant
                     { return ModifierOfLine::Underline{underline.character, underline.continuous}; },
                     [&arguments](const DiagnosticBase::InsertAfter& insertAfter) -> ModifierOfLine::variant
                     {
                         if (insertAfter.text >= 0)
                         {
                             CORAL66_ASSERT(arguments[insertAfter.text].inArgText);
                             return ModifierOfLine::InsertAfter{*arguments[insertAfter.text].inArgText};
                         }
                         return ModifierOfLine::InsertAfter{};
                     },
                     [&arguments, index](const DiagnosticBase::Annotate& annotate) -> ModifierOfLine::variant
                     {
                         CORAL66_ASSERT(arguments[annotate.text].inArgText);
                         const auto begin = arguments[index].range->first.offset;
                         const auto end = arguments[index].range->second.offset;
                         return ModifierOfLine::Annotate{*arguments[annotate.text].inArgText,
                                                         begin + (end - begin) / 2};
                     })});
        }
    }

    auto width = std::ranges::max_element(fileToMaxMinLine,
                                          [](auto&& lhs, auto&& rhs) { return lhs.second.second < rhs.second.second; })
                     ->second.second;
    width = roundUpTo(1 + static_cast<std::size_t>(std::floor(log10f(width))), 4);

    for (auto [fileId, lineBounds] : fileToMaxMinLine)
    {
        auto [minLine, maxLine] = lineBounds;
        const auto startOffset = sourceInterface.getLineStartOffset(fileId, minLine);
        const auto endOffset = sourceInterface.getLineEndOffset(fileId, maxLine);
        auto view =
            std::string_view(sourceInterface.getFiles()[fileId].source).substr(startOffset, endOffset - startOffset);
        std::vector<std::int64_t> mapping(view.size() + 1);
        std::string safeUTF8;
        safeUTF8.reserve(mapping.size());
        coral66::toSafeUTF8(view, std::back_inserter(safeUTF8), mapping.begin());
        view = safeUTF8;
        for (auto i = minLine; i <= maxLine; i++)
        {
            auto thisLineStartOffset = sourceInterface.getLineStartOffset(fileId, i);
            thisLineStartOffset += mapping[thisLineStartOffset - startOffset];
            auto thisLineEndOffset = sourceInterface.getLineEndOffset(fileId, i);
            thisLineEndOffset += mapping[thisLineEndOffset - startOffset];

            auto& modifierForLine = fileAndLineToModifiers[{fileId, i}];
            for (auto& iter : modifierForLine)
            {
                iter.fromToInLine.first += mapping[iter.fromToInLine.first - startOffset];
                iter.fromToInLine.second += mapping[iter.fromToInLine.second - startOffset];
                if (std::holds_alternative<ModifierOfLine::Annotate>(iter.value))
                {
                    auto& annotate = coral66::get<ModifierOfLine::Annotate>(iter.value);
                    if (annotate.pos >= thisLineStartOffset && annotate.pos <= thisLineEndOffset)
                    {
                        annotate.pos += mapping[annotate.pos - startOffset];
                    }
                }
            }

            std::ranges::stable_sort(modifierForLine,
                                     [](const ModifierOfLine& lhs, const ModifierOfLine& rhs)
                                     {
                                         if (std::holds_alternative<ModifierOfLine::Underline>(lhs.value)
                                             && std::holds_alternative<ModifierOfLine::Underline>(rhs.value))
                                         {
                                             return coral66::get<ModifierOfLine::Underline>(lhs.value).continuous
                                                    > coral66::get<ModifierOfLine::Underline>(rhs.value).continuous;
                                         }
                                         if (std::holds_alternative<ModifierOfLine::Underline>(lhs.value)
                                             || std::holds_alternative<ModifierOfLine::Underline>(rhs.value))
                                         {
                                             return lhs.value.index() < rhs.value.index();
                                         }
                                         // > because we want modifiers that appear further to the right to be printed
                                         // first so that they have the lowest height
                                         return lhs.fromToInLine.second > rhs.fromToInLine.second;
                                     });

            printLine(ss, i, thisLineStartOffset,
                      view.substr(thisLineStartOffset - startOffset, thisLineEndOffset - thisLineStartOffset), width,
                      modifierForLine);
        }
    }

    ss.flush();
    return {m_severity, std::move(result)};
}

void coral66::detail::Diagnostic::DiagnosticBase::evaluateFormatsInMessage(const std::string_view message,
                                                                           const std::span<Argument> arguments,
                                                                           llvm::raw_string_ostream& ss) const
{
    switch (this->m_severity)
    {
        case Severity::None: CORAL66_UNREACHABLE;
        case Severity::Error: llvm::WithColor(ss, llvm::raw_string_ostream::RED, true) << "error: "; break;
        case Severity::Warning: llvm::WithColor(ss, llvm::raw_string_ostream::MAGENTA, true) << "warning: "; break;
        case Severity::Note: llvm::WithColor(ss, llvm::raw_string_ostream::CYAN, true) << "note: "; break;
    }
    auto copy = message;
    const char* last = copy.data();
    while (const auto iter = nextArg(copy))
    {
        auto index = iter->index;
        auto mods = iter->modifier;

        ss.write(last, iter->view.data() - last);
        if (mods.empty())
        {
            CORAL66_ASSERT(arguments[index].inFormatText);
            ss << *arguments[index].inFormatText;
        }
        else
        {
            ss << arguments[index].customModifiers[iter->modifier];
        }
        last = iter->view.data() + iter->view.size();
    }
    ss.write(copy.data(), copy.size());
    if (m_severity == Severity::Warning)
    {
        ss << " [" << m_name << "]";
    }
    ss << '\n';
}

coral66::Message coral66::detail::Diagnostic::DiagnosticBase::print(std::string_view message,
                                                                    std::span<Argument> arguments) const
{
    std::string result;
    ColorStringOstream ss(result);
    llvm::WithColor(ss, llvm::raw_string_ostream::RED, true) << "coral66"

                                                                ": ";
    evaluateFormatsInMessage(message, arguments, ss);
    ss.flush();
    return {m_severity, std::move(result)};
}

std::unordered_set<std::string_view>& coral66::detail::Diagnostic::DiagnosticBase::allWarnings()
{
    static std::unordered_set<std::string_view> set;
    return set;
}

const std::unordered_set<std::string_view>& coral66::diag::getAllWarnings()
{
    return ::coral66::detail::Diagnostic::DiagnosticBase::allWarnings();
}

coral66::detail::Diagnostic::WarningRegistrar::WarningRegistrar(std::string_view name)
{
    DiagnosticBase::allWarnings().emplace(name);
}
