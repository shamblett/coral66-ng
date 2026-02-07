/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/frontend/compiler/preprocessor.hpp"

#include "lib/frontend/common/error_messages.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

namespace
{
coral66::PPSourceObject preprocess(std::string source)
{
    std::string storage;
    llvm::raw_string_ostream ss(storage);
    bool errorsOccurred = false;
    auto options = coral66::LanguageOptions::native();
    auto tokens = coral66::Lexer::tokenize(source, &options, &ss, &errorsOccurred);
    UNSCOPED_INFO(ss.str());
    REQUIRE_FALSE(errorsOccurred);
    auto result = coral66::PP::preprocess(std::move(tokens), {}, &ss, &errorsOccurred);
    UNSCOPED_INFO(ss.str());
    REQUIRE_FALSE(errorsOccurred);
    return result;
}
} // namespace

namespace coral66::Tests
{
struct ProducesPP : Catch::Matchers::MatcherBase<coral66::PPSourceObject>
{
private:
    std::string m_source;

public:
    ProducesPP(std::string source) : m_source(std::move(source)) {}

    bool match(const coral66::PPSourceObject& arg) const override
    {
        bool errors = false;
        auto options = coral66::LanguageOptions::native();
        auto ret = coral66::Lexer::tokenize(m_source, &options, nullptr, &errors);
        if (errors)
            return false;

        // Simple token comparison ignoring locations
        if (ret.data().size() != arg.data().size())
            return false;

        for (size_t i = 0; i < ret.data().size(); ++i)
        {
            if (ret.data()[i].getValue() != arg.data()[i].getValue())
                return false;
        }
        return true;
    }

    std::string describe() const override
    {
        return "produces pp tokens of:\n" + m_source;
    }
};
} // namespace coral66::Tests

using namespace coral66::Tests;

TEST_CASE("Coral DEFINE Macro", "[PP]")
{
    SECTION("Simple Substitution")
    {
        auto ret = preprocess("DEFINE ID '5'; ID");
        CHECK_THAT(ret, ProducesPP("5"));
    }

    SECTION("Multiple Defines")
    {
        auto ret = preprocess("DEFINE A '10'; DEFINE B '20'; A B");
        CHECK_THAT(ret, ProducesPP("10 20"));
    }

    SECTION("Replacement is tokenized")
    {
        // "1 + 2" should be tokenized to '1', '+', '2'
        auto ret = preprocess("DEFINE EXPR '1 + 2'; EXPR");
        CHECK_THAT(ret, ProducesPP("1 + 2"));
    }
}
