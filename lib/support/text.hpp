/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

/**
 * General text processing support
 *
 */

#include <llvm/Support/ConvertUTF.h>
#include <llvm/Support/Unicode.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

#include "constexpr.hpp"
#include "max_vector.hpp"
#include "utilities.hpp"

namespace llvm
{
class APSInt;
class APFloat;
} // namespace llvm

namespace coral66
{
bool isWhitespace(std::uint32_t c) noexcept;

template <class Iter>
std::uint8_t getNumBytesForUTF8(Iter begin, Iter end)
{
    CORAL66_ASSERT(begin != end);
    std::uint8_t des;
    const std::int8_t temp = *begin;
    std::memcpy(&des, &temp, 1);

    std::uint8_t step = 0;
    if (des < 0x80)
        step = 1;
    else if (des < 0xC0)
        step = 0;
    else if (des < 0xE0)
        step = 2;
    else if (des < 0xF0)
        step = 3;
    else if (des < 0xF8)
        step = 4;

    if (step == 0)
    {
        return 1;
    }
    for (std::uint8_t i = 1; i < step; i++)
    {
        if (begin + i == end || (*(begin + i) & 0b11000000) != 0b10000000)
        {
            return i;
        }
    }
    return step;
}

template <class OutputIter1, class OutputIterator2>
void toSafeUTF8(std::string_view const inputLine, OutputIter1 outputIter, OutputIterator2 mappingIter)
{
    constexpr static std::string_view REPLACEMENT_CHARACTER = "\xEF\xBF\xBD";
    std::int64_t delta = 0;
    const auto* sourceStart = inputLine.data();
    const auto* const sourceEnd = inputLine.data() + inputLine.size();
    while (sourceStart != sourceEnd)
    {
        do
        {
            llvm::UTF32 utf32;
            llvm::UTF32* targetStart = &utf32;
            const auto* const prev = sourceStart;
            const auto convRes = llvm::ConvertUTF8toUTF32(reinterpret_cast<const llvm::UTF8**>(&sourceStart),
                                                          reinterpret_cast<const llvm::UTF8*>(sourceEnd), &targetStart,
                                                          targetStart + 1, llvm::strictConversion);
            if (convRes != llvm::conversionOK && convRes != llvm::targetExhausted)
            {
                break;
            }
            mappingIter = std::fill_n(mappingIter, std::distance(prev, sourceStart), delta);
            if (isWhitespace(utf32) || llvm::sys::unicode::isPrintable(static_cast<int>(utf32)))
            {
                outputIter = std::copy(prev, sourceStart, outputIter);
                continue;
            }

            delta += 3 - std::distance(prev, sourceStart);
            if (utf32 <= 0x1F)
            {
                char nullControlCharacter[] = {static_cast<char>(0xE2), static_cast<char>(0x90),
                                               static_cast<char>(0x80)};
                nullControlCharacter[2] |= static_cast<char>(utf32); // NOLINT(*-narrowing-conversions)
                outputIter = std::copy(std::begin(nullControlCharacter), std::end(nullControlCharacter), outputIter);
            }
            else
            {
                outputIter = std::copy(REPLACEMENT_CHARACTER.begin(), REPLACEMENT_CHARACTER.end(), outputIter);
            }
        } while (sourceStart != sourceEnd);

        if (sourceStart != sourceEnd)
        {
            outputIter = std::copy(REPLACEMENT_CHARACTER.begin(), REPLACEMENT_CHARACTER.end(), outputIter);
            const auto step = getNumBytesForUTF8(sourceStart, sourceEnd);
            mappingIter = std::fill_n(mappingIter, step, delta);
            delta += 3 - step;
            sourceStart += step;
        }
    }

    *mappingIter = delta;
}

int unsafeCharWidth(std::uint32_t UCS);

/**
 * Returns an approximation for the width of the UTF8 text on a terminal. Passing Invalid UTF-8 or Non-printable
 * Characters is UB
 * @param text
 * @return
 */
unsigned unsafeColumnWidth(std::string_view text);

template <class BackInserter>
BackInserter stringOfSameWidth(std::string_view const original, char characterToReplace, BackInserter backInserter)
{
    auto utf8Width = unsafeColumnWidth(original);
    return std::fill_n(backInserter, utf8Width, characterToReplace);
}

inline std::string to_string(std::string_view stringView)
{
    return {stringView.begin(), stringView.end()};
}

namespace Constexpr
{
template <std::size_t maxSize>
constexpr MaxVector<char, maxSize> utf32ToUtf8(std::u32string_view const stringView)
{
    MaxVector<char, maxSize> result;
    for (char32_t code : stringView)
    {
        if (code <= 0x7F)
        {
            result.push_back(code);
        }
        else if (code <= 0x7FF)
        {
            result.push_back(0xC0 | (code >> 6));
            result.push_back(0x80 | (code & 0x3F));
        }
        else if (code <= 0xFFFF)
        {
            result.push_back(0xE0 | (code >> 12));
            result.push_back(0x80 | ((code >> 6) & 0x3F));
            result.push_back(0x80 | (code & 0x3F));
        }
        else if (code <= 0x10FFFF)
        {
            result.push_back(0xF0 | (code >> 18));
            result.push_back(0x80 | ((code >> 12) & 0x3F));
            result.push_back(0x80 | ((code >> 6) & 0x3F));
            result.push_back(0x80 | (code & 0x3F));
        }
    }
    return result;
}

} // namespace Constexpr

template <class T>
requires requires(T value)
{
    std::to_string(value);
}
std::string to_string(T value)
{
    return std::to_string(value);
}

std::string to_string(const llvm::APSInt& apsInt);

std::string to_string(const llvm::APFloat& apFloat);

} // namespace coral66
