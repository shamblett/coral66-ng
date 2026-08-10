/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "text.hpp"

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/UnicodeCharRanges.h>

bool coral66::isWhitespace(const std::uint32_t c) noexcept
{
    constexpr static llvm::sys::UnicodeCharRange UnicodeWhitespaceCharRanges[] = {
        {.Lower = 0x0085, .Upper = 0x0085}, {.Lower = 0x00A0, .Upper = 0x00A0}, {.Lower = 0x1680, .Upper = 0x1680},
        {.Lower = 0x180E, .Upper = 0x180E}, {.Lower = 0x2000, .Upper = 0x200A}, {.Lower = 0x2028, .Upper = 0x2029},
        {.Lower = 0x202F, .Upper = 0x202F}, {.Lower = 0x205F, .Upper = 0x205F}, {.Lower = 0x3000, .Upper = 0x3000}};

    static const llvm::sys::UnicodeCharSet UnicodeWhitespaceChar(UnicodeWhitespaceCharRanges);

    return c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f' || c == '\v'
           || UnicodeWhitespaceChar.contains(c);
}

int coral66::unsafeCharWidth(const std::uint32_t UCS)
{
    if (UCS == '\t')
    {
        return 4;
    }
    // Sorted list of non-spacing and enclosing combining mark intervals as
    // defined in "3.6 Combination" of
    // http://www.unicode.org/versions/Unicode6.2.0/UnicodeStandard-6.2.pdf
    static const llvm::sys::UnicodeCharRange CombiningCharacterRanges[] = {
        {.Lower = 0x0300, .Upper = 0x036F},   {.Lower = 0x0483, .Upper = 0x0489},
        {.Lower = 0x0591, .Upper = 0x05BD},   {.Lower = 0x05BF, .Upper = 0x05BF},
        {.Lower = 0x05C1, .Upper = 0x05C2},   {.Lower = 0x05C4, .Upper = 0x05C5},
        {.Lower = 0x05C7, .Upper = 0x05C7},   {.Lower = 0x0610, .Upper = 0x061A},
        {.Lower = 0x064B, .Upper = 0x065F},   {.Lower = 0x0670, .Upper = 0x0670},
        {.Lower = 0x06D6, .Upper = 0x06DC},   {.Lower = 0x06DF, .Upper = 0x06E4},
        {.Lower = 0x06E7, .Upper = 0x06E8},   {.Lower = 0x06EA, .Upper = 0x06ED},
        {.Lower = 0x0711, .Upper = 0x0711},   {.Lower = 0x0730, .Upper = 0x074A},
        {.Lower = 0x07A6, .Upper = 0x07B0},   {.Lower = 0x07EB, .Upper = 0x07F3},
        {.Lower = 0x0816, .Upper = 0x0819},   {.Lower = 0x081B, .Upper = 0x0823},
        {.Lower = 0x0825, .Upper = 0x0827},   {.Lower = 0x0829, .Upper = 0x082D},
        {.Lower = 0x0859, .Upper = 0x085B},   {.Lower = 0x08E4, .Upper = 0x08FE},
        {.Lower = 0x0900, .Upper = 0x0902},   {.Lower = 0x093A, .Upper = 0x093A},
        {.Lower = 0x093C, .Upper = 0x093C},   {.Lower = 0x0941, .Upper = 0x0948},
        {.Lower = 0x094D, .Upper = 0x094D},   {.Lower = 0x0951, .Upper = 0x0957},
        {.Lower = 0x0962, .Upper = 0x0963},   {.Lower = 0x0981, .Upper = 0x0981},
        {.Lower = 0x09BC, .Upper = 0x09BC},   {.Lower = 0x09C1, .Upper = 0x09C4},
        {.Lower = 0x09CD, .Upper = 0x09CD},   {.Lower = 0x09E2, .Upper = 0x09E3},
        {.Lower = 0x0A01, .Upper = 0x0A02},   {.Lower = 0x0A3C, .Upper = 0x0A3C},
        {.Lower = 0x0A41, .Upper = 0x0A42},   {.Lower = 0x0A47, .Upper = 0x0A48},
        {.Lower = 0x0A4B, .Upper = 0x0A4D},   {.Lower = 0x0A51, .Upper = 0x0A51},
        {.Lower = 0x0A70, .Upper = 0x0A71},   {.Lower = 0x0A75, .Upper = 0x0A75},
        {.Lower = 0x0A81, .Upper = 0x0A82},   {.Lower = 0x0ABC, .Upper = 0x0ABC},
        {.Lower = 0x0AC1, .Upper = 0x0AC5},   {.Lower = 0x0AC7, .Upper = 0x0AC8},
        {.Lower = 0x0ACD, .Upper = 0x0ACD},   {.Lower = 0x0AE2, .Upper = 0x0AE3},
        {.Lower = 0x0B01, .Upper = 0x0B01},   {.Lower = 0x0B3C, .Upper = 0x0B3C},
        {.Lower = 0x0B3F, .Upper = 0x0B3F},   {.Lower = 0x0B41, .Upper = 0x0B44},
        {.Lower = 0x0B4D, .Upper = 0x0B4D},   {.Lower = 0x0B56, .Upper = 0x0B56},
        {.Lower = 0x0B62, .Upper = 0x0B63},   {.Lower = 0x0B82, .Upper = 0x0B82},
        {.Lower = 0x0BC0, .Upper = 0x0BC0},   {.Lower = 0x0BCD, .Upper = 0x0BCD},
        {.Lower = 0x0C3E, .Upper = 0x0C40},   {.Lower = 0x0C46, .Upper = 0x0C48},
        {.Lower = 0x0C4A, .Upper = 0x0C4D},   {.Lower = 0x0C55, .Upper = 0x0C56},
        {.Lower = 0x0C62, .Upper = 0x0C63},   {.Lower = 0x0CBC, .Upper = 0x0CBC},
        {.Lower = 0x0CBF, .Upper = 0x0CBF},   {.Lower = 0x0CC6, .Upper = 0x0CC6},
        {.Lower = 0x0CCC, .Upper = 0x0CCD},   {.Lower = 0x0CE2, .Upper = 0x0CE3},
        {.Lower = 0x0D41, .Upper = 0x0D44},   {.Lower = 0x0D4D, .Upper = 0x0D4D},
        {.Lower = 0x0D62, .Upper = 0x0D63},   {.Lower = 0x0DCA, .Upper = 0x0DCA},
        {.Lower = 0x0DD2, .Upper = 0x0DD4},   {.Lower = 0x0DD6, .Upper = 0x0DD6},
        {.Lower = 0x0E31, .Upper = 0x0E31},   {.Lower = 0x0E34, .Upper = 0x0E3A},
        {.Lower = 0x0E47, .Upper = 0x0E4E},   {.Lower = 0x0EB1, .Upper = 0x0EB1},
        {.Lower = 0x0EB4, .Upper = 0x0EB9},   {.Lower = 0x0EBB, .Upper = 0x0EBC},
        {.Lower = 0x0EC8, .Upper = 0x0ECD},   {.Lower = 0x0F18, .Upper = 0x0F19},
        {.Lower = 0x0F35, .Upper = 0x0F35},   {.Lower = 0x0F37, .Upper = 0x0F37},
        {.Lower = 0x0F39, .Upper = 0x0F39},   {.Lower = 0x0F71, .Upper = 0x0F7E},
        {.Lower = 0x0F80, .Upper = 0x0F84},   {.Lower = 0x0F86, .Upper = 0x0F87},
        {.Lower = 0x0F8D, .Upper = 0x0F97},   {.Lower = 0x0F99, .Upper = 0x0FBC},
        {.Lower = 0x0FC6, .Upper = 0x0FC6},   {.Lower = 0x102D, .Upper = 0x1030},
        {.Lower = 0x1032, .Upper = 0x1037},   {.Lower = 0x1039, .Upper = 0x103A},
        {.Lower = 0x103D, .Upper = 0x103E},   {.Lower = 0x1058, .Upper = 0x1059},
        {.Lower = 0x105E, .Upper = 0x1060},   {.Lower = 0x1071, .Upper = 0x1074},
        {.Lower = 0x1082, .Upper = 0x1082},   {.Lower = 0x1085, .Upper = 0x1086},
        {.Lower = 0x108D, .Upper = 0x108D},   {.Lower = 0x109D, .Upper = 0x109D},
        {.Lower = 0x135D, .Upper = 0x135F},   {.Lower = 0x1712, .Upper = 0x1714},
        {.Lower = 0x1732, .Upper = 0x1734},   {.Lower = 0x1752, .Upper = 0x1753},
        {.Lower = 0x1772, .Upper = 0x1773},   {.Lower = 0x17B4, .Upper = 0x17B5},
        {.Lower = 0x17B7, .Upper = 0x17BD},   {.Lower = 0x17C6, .Upper = 0x17C6},
        {.Lower = 0x17C9, .Upper = 0x17D3},   {.Lower = 0x17DD, .Upper = 0x17DD},
        {.Lower = 0x180B, .Upper = 0x180D},   {.Lower = 0x18A9, .Upper = 0x18A9},
        {.Lower = 0x1920, .Upper = 0x1922},   {.Lower = 0x1927, .Upper = 0x1928},
        {.Lower = 0x1932, .Upper = 0x1932},   {.Lower = 0x1939, .Upper = 0x193B},
        {.Lower = 0x1A17, .Upper = 0x1A18},   {.Lower = 0x1A56, .Upper = 0x1A56},
        {.Lower = 0x1A58, .Upper = 0x1A5E},   {.Lower = 0x1A60, .Upper = 0x1A60},
        {.Lower = 0x1A62, .Upper = 0x1A62},   {.Lower = 0x1A65, .Upper = 0x1A6C},
        {.Lower = 0x1A73, .Upper = 0x1A7C},   {.Lower = 0x1A7F, .Upper = 0x1A7F},
        {.Lower = 0x1B00, .Upper = 0x1B03},   {.Lower = 0x1B34, .Upper = 0x1B34},
        {.Lower = 0x1B36, .Upper = 0x1B3A},   {.Lower = 0x1B3C, .Upper = 0x1B3C},
        {.Lower = 0x1B42, .Upper = 0x1B42},   {.Lower = 0x1B6B, .Upper = 0x1B73},
        {.Lower = 0x1B80, .Upper = 0x1B81},   {.Lower = 0x1BA2, .Upper = 0x1BA5},
        {.Lower = 0x1BA8, .Upper = 0x1BA9},   {.Lower = 0x1BAB, .Upper = 0x1BAB},
        {.Lower = 0x1BE6, .Upper = 0x1BE6},   {.Lower = 0x1BE8, .Upper = 0x1BE9},
        {.Lower = 0x1BED, .Upper = 0x1BED},   {.Lower = 0x1BEF, .Upper = 0x1BF1},
        {.Lower = 0x1C2C, .Upper = 0x1C33},   {.Lower = 0x1C36, .Upper = 0x1C37},
        {.Lower = 0x1CD0, .Upper = 0x1CD2},   {.Lower = 0x1CD4, .Upper = 0x1CE0},
        {.Lower = 0x1CE2, .Upper = 0x1CE8},   {.Lower = 0x1CED, .Upper = 0x1CED},
        {.Lower = 0x1CF4, .Upper = 0x1CF4},   {.Lower = 0x1DC0, .Upper = 0x1DE6},
        {.Lower = 0x1DFC, .Upper = 0x1DFF},   {.Lower = 0x20D0, .Upper = 0x20F0},
        {.Lower = 0x2CEF, .Upper = 0x2CF1},   {.Lower = 0x2D7F, .Upper = 0x2D7F},
        {.Lower = 0x2DE0, .Upper = 0x2DFF},   {.Lower = 0x302A, .Upper = 0x302D},
        {.Lower = 0x3099, .Upper = 0x309A},   {.Lower = 0xA66F, .Upper = 0xA672},
        {.Lower = 0xA674, .Upper = 0xA67D},   {.Lower = 0xA69F, .Upper = 0xA69F},
        {.Lower = 0xA6F0, .Upper = 0xA6F1},   {.Lower = 0xA802, .Upper = 0xA802},
        {.Lower = 0xA806, .Upper = 0xA806},   {.Lower = 0xA80B, .Upper = 0xA80B},
        {.Lower = 0xA825, .Upper = 0xA826},   {.Lower = 0xA8C4, .Upper = 0xA8C4},
        {.Lower = 0xA8E0, .Upper = 0xA8F1},   {.Lower = 0xA926, .Upper = 0xA92D},
        {.Lower = 0xA947, .Upper = 0xA951},   {.Lower = 0xA980, .Upper = 0xA982},
        {.Lower = 0xA9B3, .Upper = 0xA9B3},   {.Lower = 0xA9B6, .Upper = 0xA9B9},
        {.Lower = 0xA9BC, .Upper = 0xA9BC},   {.Lower = 0xAA29, .Upper = 0xAA2E},
        {.Lower = 0xAA31, .Upper = 0xAA32},   {.Lower = 0xAA35, .Upper = 0xAA36},
        {.Lower = 0xAA43, .Upper = 0xAA43},   {.Lower = 0xAA4C, .Upper = 0xAA4C},
        {.Lower = 0xAAB0, .Upper = 0xAAB0},   {.Lower = 0xAAB2, .Upper = 0xAAB4},
        {.Lower = 0xAAB7, .Upper = 0xAAB8},   {.Lower = 0xAABE, .Upper = 0xAABF},
        {.Lower = 0xAAC1, .Upper = 0xAAC1},   {.Lower = 0xAAEC, .Upper = 0xAAED},
        {.Lower = 0xAAF6, .Upper = 0xAAF6},   {.Lower = 0xABE5, .Upper = 0xABE5},
        {.Lower = 0xABE8, .Upper = 0xABE8},   {.Lower = 0xABED, .Upper = 0xABED},
        {.Lower = 0xFB1E, .Upper = 0xFB1E},   {.Lower = 0xFE00, .Upper = 0xFE0F},
        {.Lower = 0xFE20, .Upper = 0xFE26},   {.Lower = 0x101FD, .Upper = 0x101FD},
        {.Lower = 0x10A01, .Upper = 0x10A03}, {.Lower = 0x10A05, .Upper = 0x10A06},
        {.Lower = 0x10A0C, .Upper = 0x10A0F}, {.Lower = 0x10A38, .Upper = 0x10A3A},
        {.Lower = 0x10A3F, .Upper = 0x10A3F}, {.Lower = 0x11001, .Upper = 0x11001},
        {.Lower = 0x11038, .Upper = 0x11046}, {.Lower = 0x11080, .Upper = 0x11081},
        {.Lower = 0x110B3, .Upper = 0x110B6}, {.Lower = 0x110B9, .Upper = 0x110BA},
        {.Lower = 0x11100, .Upper = 0x11102}, {.Lower = 0x11127, .Upper = 0x1112B},
        {.Lower = 0x1112D, .Upper = 0x11134}, {.Lower = 0x11180, .Upper = 0x11181},
        {.Lower = 0x111B6, .Upper = 0x111BE}, {.Lower = 0x116AB, .Upper = 0x116AB},
        {.Lower = 0x116AD, .Upper = 0x116AD}, {.Lower = 0x116B0, .Upper = 0x116B5},
        {.Lower = 0x116B7, .Upper = 0x116B7}, {.Lower = 0x16F8F, .Upper = 0x16F92},
        {.Lower = 0x1D167, .Upper = 0x1D169}, {.Lower = 0x1D17B, .Upper = 0x1D182},
        {.Lower = 0x1D185, .Upper = 0x1D18B}, {.Lower = 0x1D1AA, .Upper = 0x1D1AD},
        {.Lower = 0x1D242, .Upper = 0x1D244}, {.Lower = 0xE0100, .Upper = 0xE01EF},
    };
    static const llvm::sys::UnicodeCharSet CombiningCharacters(*CombiningCharacterRanges);

    if (CombiningCharacters.contains(UCS))
        return 0;

    static const llvm::sys::UnicodeCharRange DoubleWidthCharacterRanges[] = {
        // Hangul Jamo
        {.Lower = 0x1100, .Upper = 0x11FF},
        // Deprecated fullwidth angle brackets
        {.Lower = 0x2329, .Upper = 0x232A},
        // CJK Misc, CJK Unified Ideographs, Yijing Hexagrams, Yi
        // excluding U+303F (IDEOGRAPHIC HALF FILL SPACE)
        {.Lower = 0x2E80, .Upper = 0x303E},
        {.Lower = 0x3040, .Upper = 0xA4CF},
        // Hangul
        {.Lower = 0xAC00, .Upper = 0xD7A3},
        {.Lower = 0xD7B0, .Upper = 0xD7C6},
        {.Lower = 0xD7CB, .Upper = 0xD7FB},
        // CJK Unified Ideographs
        {.Lower = 0xF900, .Upper = 0xFAFF},
        // Vertical forms
        {.Lower = 0xFE10, .Upper = 0xFE19},
        // CJK Compatibility Forms + Small Form Variants
        {.Lower = 0xFE30, .Upper = 0xFE6F},
        // Fullwidth forms
        {.Lower = 0xFF01, .Upper = 0xFF60},
        {.Lower = 0xFFE0, .Upper = 0xFFE6},
        // CJK Unified Ideographs
        {.Lower = 0x20000, .Upper = 0x2A6DF},
        {.Lower = 0x2A700, .Upper = 0x2B81F},
        {.Lower = 0x2F800, .Upper = 0x2FA1F}};
    static const llvm::sys::UnicodeCharSet DoubleWidthCharacters(DoubleWidthCharacterRanges);

    if (DoubleWidthCharacters.contains(UCS))
        return 2;
    return 1;
}

unsigned coral66::unsafeColumnWidth(std::string_view const text)
{
    unsigned int columnWidth = 0;
    unsigned int length;
    for (size_t i = 0, e = text.size(); i < e; i += length)
    {
        length = getNumBytesForUTF8(text.begin() + i, text.end());
        std::array<llvm::UTF32, 1> buf{};
        const auto* start = reinterpret_cast<const llvm::UTF8*>(text.data() + i);
        llvm::UTF32* target = buf.data();
        if (llvm::conversionOK
            != ConvertUTF8toUTF32(&start, start + length, &target, target + 1, llvm::strictConversion))
            CORAL66_UNREACHABLE;
        const int Width = unsafeCharWidth(buf[0]);
        columnWidth += Width;
    }
    return columnWidth;
}

std::string coral66::to_string(const llvm::APSInt& apsInt)
{
    return llvm::toString(apsInt, 10);
}

std::string coral66::to_string(const llvm::APFloat& apFloat)
{
    llvm::SmallString<25> number;
    apFloat.toString(number, 0, 3, false);
    return {number.begin(), number.end()};
}
