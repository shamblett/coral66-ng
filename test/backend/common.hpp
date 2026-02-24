/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <llvm/IR/Module.h>

#include <regex>

#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "lib/frontend/semantics/semantics.hpp"
#include "lib/frontend/lexer/lexer.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/support/function_ref.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

inline coral66::Semantics::Program generateProgram(std::string source)
{
    bool errors = false;
    auto pptokens = coral66::lexer::tokenize(std::move(source), &llvm::errs(), &errors);
    REQUIRE_FALSE(errors);
    pptokens = coral66::pp::preprocess(std::move(pptokens), &llvm::errs(), &errors);
    REQUIRE_FALSE(errors);
    auto ctokens = coral66::lexer::toCTokens(pptokens, &llvm::errs(), &errors);
    REQUIRE_FALSE(errors);
    auto tree = coral66::Parser::parseProgram(ctokens, &llvm::errs(), &errors);
    REQUIRE_FALSE(errors);
    auto program = coral66::Semantics::analyse(tree, std::move(ctokens), &llvm::errs(), &errors);
    REQUIRE_FALSE(errors);
    return program;
}

namespace coral66::Tests
{
namespace detail
{
void computeAndGet(std::unique_ptr<llvm::Module>&& module, std::string_view functionName,
                   coral66::FunctionRef<void(std::uintptr_t)> symbolCallback);

template <class F>
struct return_type;

template <class R, class... A>
struct return_type<R(A...)>
{
    typedef R type;
};

template <class R, class... A>
struct return_type<R(A..., ...)>
{
    typedef R type;
};

} // namespace detail

template <class Fn, class... Args>
auto computeInJIT(std::unique_ptr<llvm::Module>&& module, std::string_view functionName, Args&&... args)
{
    if constexpr (std::is_void_v<typename detail::return_type<Fn>::type>)
    {
        detail::computeAndGet(std::move(module), functionName, [&](std::uintptr_t symbol)
                              { reinterpret_cast<Fn*>(symbol)(std::forward<Args&&>(args)...); });
    }
    else
    {
        typename detail::return_type<Fn>::type result;
        detail::computeAndGet(std::move(module), functionName, [&](std::uintptr_t symbol)
                              { result = reinterpret_cast<Fn*>(symbol)(std::forward<Args&&>(args)...); });
        return result;
    }
}

inline std::string printModule(const llvm::Module& arg)
{
    std::string s;
    llvm::raw_string_ostream ss(s);
    arg.print(ss, nullptr);
    ss.flush();
    return s;
}

class ContainsIR : public Catch::Matchers::MatcherBase<llvm::Module>
{
    std::string m_pattern;

public:
    explicit ContainsIR(std::string pattern) : m_pattern("(.*)" + pattern + "(.*|\n|$)") {}

    bool match(const llvm::Module& module) const override
    {
        auto string = printModule(module);
        std::regex pattern(m_pattern);
        auto result = std::regex_search(string, pattern);
        return result;
    }

protected:
    std::string describe() const override
    {
        return " contains " + m_pattern;
    }
};

} // namespace coral66::Tests

using namespace coral66::Tests;

namespace Catch
{
template <>
struct StringMaker<llvm::Module>
{
    static std::string convert(const llvm::Module& module)
    {
        std::string s;
        {
            llvm::raw_string_ostream ss(s);
            module.print(ss, nullptr);
        }
        return "'" + s + "'";
    }
};

} // namespace Catch
