/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include "test/vendor/catch2v3/catch_amalgamated.hpp"

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/APSInt.h>

#include "lib/frontend/common/Diagnostic.hpp"
#include "lib/frontend/common/CustomDiagnostics.hpp"
#include "lib/frontend/compiler/Semantics.hpp"
#include "lib/frontend/compiler/Preprocessor.hpp"
#include "lib/support/Filesystem.hpp"
#include "lib/support/Text.hpp"

#include <numeric>

#include "TestTargets.hpp"

namespace coral66::Tests
{
namespace detail
{
template <class Diagnostic, class... Args>
std::string args(const Diagnostic&, Args&&... args)
{
    std::array<std::string, sizeof...(Args)> strArgs = {{coral66::to_string(args)...}};
    static_assert(coral66::detail::Diagnostic::getBiggestPercentArg(Diagnostic::getFormat()) == sizeof...(Args));
    std::string result;
    std::string_view stringView = Diagnostic::getFormat();
    const char* last = stringView.data();
    while (auto iter = ::coral66::detail::Diagnostic::nextArg(stringView))
    {
        auto view = iter->view;
        result += std::string_view(last, view.data() - last);
        last = view.data() + view.size();
        auto mod = iter->modifier;
        auto index = iter->index;
        if (mod != "s")
        {
            result += strArgs[index];
        }
        else
        {
            if (strArgs[index] != "1")
            {
                result += "s";
            }
        }
    }
    result += stringView;
    return result;
}
} // namespace detail

struct ProducesError : Catch::Matchers::StringMatcherBase
{
    ProducesError(const std::string& comparator)
        : StringMatcherBase("contains", Catch::Matchers::CasedString("error: " + comparator, Catch::CaseSensitive::Yes))
    {
    }

    template <class Diagnostic, class... Args>
    ProducesError(const Diagnostic& diagnostic, Args&&... args)
        : ProducesError(detail::args(diagnostic, std::forward<Args>(args)...))
    {
    }

    bool match(const std::string& source) const override
    {
        return Catch::Matchers::StringContainsMatcher(m_comparator).match(source);
    }
};

struct ProducesNoErrors : Catch::Matchers::StringMatcherBase
{
    ProducesNoErrors() : StringMatcherBase("contains", Catch::Matchers::CasedString("error: ", Catch::CaseSensitive::Yes))
    {
    }

    bool match(const std::string& source) const override
    {
        return !Catch::Matchers::StringContainsMatcher(m_comparator).match(source);
    }

    std::string describe() const override
    {
        return "produces no errors";
    }
};

struct ProducesNote : Catch::Matchers::StringMatcherBase
{
    ProducesNote(const std::string& comparator)
        : StringMatcherBase("contains", Catch::Matchers::CasedString("note: " + comparator, Catch::CaseSensitive::Yes))
    {
    }

    template <class Diagnostic, class... Args>
    ProducesNote(const Diagnostic& diagnostic, Args&&... args)
        : ProducesNote(detail::args(diagnostic, std::forward<Args>(args)...))
    {
    }

    bool match(const std::string& source) const override
    {
        return Catch::Matchers::StringContainsMatcher(m_comparator).match(source);
    }
};

struct ProducesNoNotes : Catch::Matchers::StringMatcherBase
{
    ProducesNoNotes() : StringMatcherBase("contains", Catch::Matchers::CasedString("note: ", Catch::CaseSensitive::Yes)) {}

    bool match(const std::string& source) const override
    {
        return !Catch::Matchers::StringContainsMatcher(m_comparator).match(source);
    }

    std::string describe() const override
    {
        return "produces no notes";
    }
};

struct ProducesWarning : Catch::Matchers::StringMatcherBase
{
    ProducesWarning(const std::string& comparator)
        : StringMatcherBase("contains", Catch::Matchers::CasedString("warning: " + comparator, Catch::CaseSensitive::Yes))
    {
    }

    template <class Diagnostic, class... Args>
    ProducesWarning(const Diagnostic& diagnostic, Args&&... args)
        : ProducesWarning(detail::args(diagnostic, std::forward<Args>(args)...))
    {
    }

    bool match(const std::string& source) const override
    {
        return Catch::Matchers::StringContainsMatcher(m_comparator).match(source);
    }
};

struct ProducesNoWarnings : Catch::Matchers::StringMatcherBase
{
    ProducesNoWarnings()
        : StringMatcherBase("contains", Catch::Matchers::CasedString("warning: ", Catch::CaseSensitive::Yes))
    {
    }

    bool match(const std::string& source) const override
    {
        return !Catch::Matchers::StringContainsMatcher(m_comparator).match(source);
    }

    std::string describe() const override
    {
        return "produces no warnings";
    }
};

// Matcher that works almost like == of two strings except it doesn't care of any leading whitespaces per line
struct ProducesLines : Catch::Matchers::MatcherBase<std::string_view>
{
    ProducesLines(std::string_view text)
    {
        std::size_t result = 0;
        while (true)
        {
            auto newline = text.find('\n', result);
            if (newline == std::string_view::npos)
            {
                m_linesTrimmed.emplace_back(trimEnd(text.substr(result)));
                break;
            }
            m_linesTrimmed.emplace_back(trimEnd(text.substr(result, newline - result)));
            result = newline + 1;
        }
    }

    std::string describe() const override
    {
        return "lines match \""
               + std::accumulate(m_linesTrimmed.begin(), m_linesTrimmed.end(), std::string(),
                                 [first = true](const std::string& lhs, const std::string& rhs) mutable {
                                     if (!first)
                                     {
                                         return lhs + '\n' + rhs;
                                     }

                                     first = false;
                                     return rhs;
                                 })
               + '"';
    }

    bool match(const std::string_view& source) const override
    {
        std::size_t i = 0;
        std::size_t result = 0;
        while (i < m_linesTrimmed.size())
        {
            auto newline = source.find('\n', result);
            if (newline == std::string_view::npos)
            {
                return trimEnd(source.substr(result)) == m_linesTrimmed[i] && i + 1 == m_linesTrimmed.size();
            }
            if (trimEnd(source.substr(result, newline - result)) != m_linesTrimmed[i])
            {
                return false;
            }
            result = newline + 1;
            i++;
        }
        auto leftOver = source.substr(result);
        return std::all_of(leftOver.begin(), leftOver.end(), [](unsigned char c) { return std::isspace(c & 0x7F); });
    }

private:
    static std::string_view trimEnd(std::string_view text)
    {
        // ASCII should hopefully suffice for our tests... Otherwise I will have to properly parse it to UTF32
        // and use one of the whitespace sets of the Lexer to match whitespace characters
        auto end =
            std::find_if(std::make_reverse_iterator(text.end()), std::make_reverse_iterator(text.begin()), [](char c) {
                return c != ' ' && c != '\t';
            }).base();
        return text.substr(0, end - text.begin());
    }

    std::vector<std::string> m_linesTrimmed;
};

struct FileScope
{
    coral66::fs::path m_path;

public:
    FileScope(coral66::fs::path path) : m_path(std::move(path)) {}

    ~FileScope();

    FileScope(const FileScope&) = delete;
    FileScope& operator=(const FileScope&) = delete;
    FileScope(FileScope&&) = delete;
    FileScope& operator=(FileScope&&) = delete;
};

[[nodiscard]] FileScope createInclude(std::string_view path, std::string_view content);

struct EndTest
{
    coral66::fs::path path;
    ~EndTest();
};

[[nodiscard]] EndTest enableParallelization();

} // namespace coral66::Tests

// IT'S ONLY FOR TESTS I SWEAR. It's in a namepspace for mangling
using namespace coral66::Tests;

namespace Catch
{
template <>
struct StringMaker<llvm::APSInt>
{
    static std::string convert(const llvm::APSInt& value)
    {
        return "APSInt{" + std::to_string(value.getBitWidth()) + " bits, " + std::to_string(value.getExtValue()) + "}";
    }
};

template <>
struct StringMaker<llvm::APInt>
{
    static std::string convert(const llvm::APSInt& value)
    {
        return "APInt{" + std::to_string(value.getBitWidth()) + " bits, " + std::to_string(value.getExtValue()) + "}";
    }
};

template <>
struct StringMaker<coral66::Semantics::Type>
{
    static std::string convert([[maybe_unused]]const coral66::Semantics::Type& type)
    {
        return "Type";
    }
};

template <>
struct StringMaker<coral66::PPSourceObject>
{
    static std::string convert(const coral66::PPSourceObject& sourceObject)
    {
        return "'"
               + coral66::PP::reconstruct(sourceObject.data().data(),
                                      sourceObject.data().data() + sourceObject.data().size(), sourceObject)
               + "'";
    }
};
} // namespace Catch
