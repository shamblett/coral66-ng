/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include <iostream>
#include <utility>

#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

namespace
{
coral66::PPSourceObject preprocess(std::string source)
{
    std::string storage;
    llvm::raw_string_ostream ss(storage);
    bool errorsOccurred = false;
    auto tokens = coral66::lexer::tokenize(std::move(source), &ss, &errorsOccurred);
    UNSCOPED_INFO(ss.str());
    REQUIRE_FALSE(errorsOccurred);
    auto result = coral66::pp::preprocess(std::move(tokens), &ss, &errorsOccurred);
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
    explicit ProducesPP(std::string source) : m_source(std::move(source)) {}

    bool match(const PPSourceObject& arg) const override
    {
        bool errors = false;
        auto ret = coral66::lexer::tokenize(m_source, nullptr, &errors);
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

TEST_CASE("Coral DEFINE Macro Syntax", "[PP]")
{
    SECTION("No Semicolon")
    {
        std::string source = "DEFINE ID '5' "
                             "ID";
        std::string storage;
        llvm::raw_string_ostream ss(storage);
        bool errorsOccurred = false;
        auto tokens = coral66::lexer::tokenize(std::move(source), &ss, &errorsOccurred);
        UNSCOPED_INFO(ss.str());
        REQUIRE_FALSE(errorsOccurred);
        auto result = coral66::pp::preprocess(std::move(tokens), &ss, &errorsOccurred);
        CHECK(errorsOccurred);
        CHECK(ss.str().contains(";"));
    }
}

TEST_CASE("Coral DELETE Macro Syntax", "[PP]")
{
    SECTION("No Semicolon")
    {
        std::string source = "DELETE ID ";
        std::string storage;
        llvm::raw_string_ostream ss(storage);
        bool errorsOccurred = false;
        auto tokens = coral66::lexer::tokenize(std::move(source), &ss, &errorsOccurred);
        UNSCOPED_INFO(ss.str());
        REQUIRE_FALSE(errorsOccurred);
        auto result = coral66::pp::preprocess(std::move(tokens), &ss, &errorsOccurred);
        CHECK(errorsOccurred);
        CHECK(ss.str().contains(";"));
    }
}

//
// Section 11.2.1
//
TEST_CASE("Coral DEFINE Macro Substitution", "[PP]")
{
    SECTION("Simple Value Substitution")
    {
        auto ret = preprocess("DEFINE ID \"5\"; "
                              "INTEGER A; "
                              "A := ID;");
        CHECK_THAT(ret, ProducesPP("DEFINE ID \"5\"; "
                                   "INTEGER A; "
                                   "A := 5;"));
    }

    SECTION("Simple Value Substitution Lowercase Identifiers")
    {
        auto ret = preprocess("DEFINE id \"5\"; "
                              "INTEGER a; "
                              "a := id;");
        CHECK_THAT(ret, ProducesPP("DEFINE id \"5\"; "
                                   "INTEGER a; "
                                   "a := 5;"));
    }

    SECTION("Simple String Substitution")
    {
        auto ret = preprocess("DEFINE ID \"5 + 1\"; "
                              "INTEGER A; "
                              "A := ID;");
        CHECK_THAT(ret, ProducesPP("DEFINE ID \"5 + 1\"; "
                                   "INTEGER A; "
                                   "A := 5 + 1;"));
    }

    SECTION("Simple String Substitution Nested Strings")
    {
        auto ret = preprocess("DEFINE ID \"5 + \"1\"\"; "
                              "INTEGER A; "
                              "A := ID;");
        CHECK_THAT(ret, ProducesPP("DEFINE ID \"5 + \"1\"\"; "
                                   "INTEGER A; "
                                   "A := 5 + \"1\";"));
    }
}

//
// Section 11.2.2
//
TEST_CASE("Coral DEFINE Macro Substitution With Parameters", "[PP]")
{
    SECTION("Single Parameter")
    {
        auto ret = preprocess("DEFINE ID(p1) \"5 + p1\"; "
                              "INTEGER A; "
                              "A := ID(1);");
        CHECK_THAT(ret, ProducesPP("DEFINE ID(p1) \"5 + p1\"; "
                                   "INTEGER A; "
                                   "A := 5 + 1;"));
    }

    SECTION("Multiple Parameters")
    {
        auto ret = preprocess("DEFINE ID(p1, p2) \"p2 + p1\"; "
                              "INTEGER A; "
                              "A := ID(1, 5);");
        CHECK_THAT(ret, ProducesPP("DEFINE ID(p1, p2) \"p2 + p1\"; "
                                   "INTEGER A; "
                                   "A := 5 + 1;"));
    }
}

//
// Section 11.2.3
//
TEST_CASE("Coral DEFINE Macro Substitution Nested", "[PP]")
{
    SECTION("Single Level")
    {
        auto ret = preprocess("DEFINE INNER \"5 + 1\"; "
                              "DEFINE OUTER \"3 + INNER\"; "
                              "INTEGER A; "
                              "A := OUTER;");
        CHECK_THAT(ret, ProducesPP("DEFINE INNER \"5 + 1\"; "
                              "DEFINE OUTER \"3 + INNER\"; "
                              "INTEGER A; "
                              "A := 3 + 5 + 1;"));
    }

    SECTION("Deferred Expansion")
    {
        auto ret = preprocess("DEFINE OUTER \"3 + INNER\"; "
                              "DEFINE INNER \"5 + 1\"; "
                              "INTEGER A; "
                              "A := OUTER;");
        CHECK_THAT(ret, ProducesPP("DEFINE OUTER \"3 + INNER\"; "
                              "DEFINE INNER \"5 + 1\"; "
                              "INTEGER A; "
                              "A := 3 + 5 + 1;"));
    }
}
