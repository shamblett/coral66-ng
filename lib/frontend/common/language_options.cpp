/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "language_options.hpp"

#include <limits>
#include <numeric>

#include "diagnostic.hpp"
#include "lib/support/Utilities.hpp"
#include "targets/x86_target_features.hpp"

namespace
{
std::unique_ptr<coral66::TargetFeatures> tripleToFeatures(const coral66::Triple& triple)
{
    if (triple.getArchitecture() == coral66::Architecture::x86_64)
    {
        auto result = std::make_unique<coral66::x86_target_features>();
        result->setFeature(coral66::TargetFeatures::SSE, true);
        result->setFeature(coral66::TargetFeatures::SSE2, true);
        return result;
    }
    return std::make_unique<coral66::DefaultTargetFeatures>();
}
} // namespace

coral66::LanguageOptions coral66::LanguageOptions::native(Language language)
{
    auto temp = coral66::LanguageOptions{
        language,
        Triple::native(),
        sizeof(bool),
        std::is_signed_v<char>,
        // wchar_t is a builtin type in C++, can't use underlyingType<wchar_t>
        UnderlyingType ::Int,
        sizeof(short),
        sizeof(int),
        sizeof(long),
        alignof(long long),
        alignof(double),
        // ReSharper disable once CppDFAConstantFunctionResult
        []() -> std::uint8_t
        {
            switch (std::numeric_limits<long double>::digits)
            {
                case 64: return 80;
                default: CORAL66_UNREACHABLE;
            }
        }(),
        alignof(long double),
        sizeof(void*),
        // ReSharper disable once CppCompileTimeConstantCanBeReplacedWithBooleanConstant
        0,
        underlyingType<ptrdiff_t>(),
        underlyingType<size_t>(),
#ifdef __SIZEOF_INT128__
        true,
#else
        false,
#endif
    };
    temp.enabledWarnings = coral66::diag::getAllWarnings();
    temp.targetFeatures = tripleToFeatures(temp.triple);
    return temp;
}

coral66::LanguageOptions coral66::LanguageOptions::fromTriple(Triple const triple, Language const language)
{
    LanguageOptions options = native(language);
    options.triple = triple;
    options.targetFeatures = tripleToFeatures(triple);
    options.enabledWarnings = coral66::diag::getAllWarnings();
    options.language = language;
    options.sizeOfUnderlineBool = 1;
    options.charIsSigned = true;
    options.wcharUnderlyingType = UnderlyingType::Int;
    options.sizeOfShort = 2;
    options.sizeOfInt = 4;
    options.sizeOfLong = 8;
    options.alignOfDouble = 8;
    options.alignOfLongDouble = 16;
    options.sizeOfLongDoubleBits = 80;
    options.sizeOfVoidStar = 8;
    options.discreteBitfields = false;
    options.ptrdiffType = UnderlyingType ::Long;
    options.sizeTType = UnderlyingType ::UnsignedLong;
    options.int128Enabled = options.sizeOfVoidStar >= 8;
    if (triple.getArchitecture() == Architecture::x86_64)
    {
        options.vaListKind = BuiltInVaList::x86_64ABI;
    }
    return options;
}

bool coral66::LanguageOptions::isSigned(coral66::LanguageOptions::UnderlyingType const type)
{
    switch (type)
    {
        case UnderlyingType::UnsignedShort:
        case UnderlyingType::UnsignedInt:
        case UnderlyingType::UnsignedLong:
        case UnderlyingType::UnsignedLongLong: return false;
        case UnderlyingType::Int:
        case UnderlyingType::Long:
        case UnderlyingType::LongLong: return true;
    }
    CORAL66_UNREACHABLE;
}

std::uint8_t coral66::LanguageOptions::sizeOf(coral66::LanguageOptions::UnderlyingType const type) const
{
    switch (type)
    {
        case UnderlyingType::UnsignedShort: return sizeOfShort;
        case UnderlyingType::Int:
        case UnderlyingType::UnsignedInt: return sizeOfInt;
        case UnderlyingType::Long:
        case UnderlyingType::UnsignedLong: return sizeOfLong;
        case UnderlyingType::LongLong:
        case UnderlyingType::UnsignedLongLong: return 8;
    }
    CORAL66_UNREACHABLE;
}

std::uint8_t coral66::LanguageOptions::alignOf(coral66::LanguageOptions::UnderlyingType const type) const
{
    switch (type)
    {
        case UnderlyingType::UnsignedShort: return sizeOfShort;
        case UnderlyingType::Int:
        case UnderlyingType::UnsignedInt: return sizeOfInt;
        case UnderlyingType::Long:
        case UnderlyingType::UnsignedLong: return sizeOfLong;
        case UnderlyingType::LongLong:
        case UnderlyingType::UnsignedLongLong: return alignOfLongLong;
    }
    CORAL66_UNREACHABLE;
}

std::string_view coral66::LanguageOptions::string(coral66::LanguageOptions::UnderlyingType const type)
{
    switch (type)
    {
        case UnderlyingType::UnsignedShort: return "unsigned short";
        case UnderlyingType::Int: return "int";
        case UnderlyingType::UnsignedInt: return "unsigned int";
        case UnderlyingType::Long: return "long";
        case UnderlyingType::UnsignedLong: return "unsigned long";
        case UnderlyingType::LongLong: return "long long";
        case UnderlyingType::UnsignedLongLong: return "unsigned long long";
    }
    CORAL66_UNREACHABLE;
}

bool coral66::DefaultTargetFeatures::hasFeature(coral66::TargetFeatures::Features) const
{
    return false;
}

bool coral66::DefaultTargetFeatures::setFeature(coral66::TargetFeatures::Features, bool)
{
    return false;
}
