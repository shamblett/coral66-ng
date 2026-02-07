/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/main/command_line.hpp"

#include "lib/frontend/common/error_messages.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

namespace
{
CORAL66_CLI_OPT(COMPILE_ONLY, ("-c", "--compile"))("Stop after compiling object files");
CORAL66_CLI_OPT(INCLUDES, ("-I<dir>", "--include-directory <dir>", "--include-directory=<dir>"),
                (std::string_view, dir))
("Additional include directories", coral66::CLIMultiArg::List);

CORAL66_CLI_OPT(STRING_ARG, ("--prefix=<arg>", "--prefix <arg>"), (std::string, arg))("A std::string arg");
CORAL66_CLI_OPT(STRING_VIEW_ARG, ("--prefix=<arg>", "--prefix <arg>"),
                (std::string_view, arg))("A std::string_view arg");
CORAL66_CLI_OPT(OPT, ("-O<level>", "-O", "--optimize", "--optimize=<level>"),
                (std::uint8_t, level))("Optimization level");
CORAL66_CLI_OPT(OPT_CHAR, ("-O<level>", "-O", "--optimize", "--optimize=<level>"), (coral66::cli::Char, level))
("Optimization level");
CORAL66_CLI_OPT(PIE, ("-f[no-]pie"))("Position independent executable");
} // namespace

using namespace coral66::Errors::CLI;

TEST_CASE("Commandline without args", "[cli]")
{
    SECTION("Appears once")
    {
        SECTION("-c alternative")
        {
            std::vector<std::string_view> elements = {"awdawd", "-c", "wadawdaw"};
            auto cli = coral66::parseCommandLine<COMPILE_ONLY>(elements);
            CHECK(cli.get<COMPILE_ONLY>());
            CHECK_THAT(cli.getUnrecognized(),
                       Catch::Matchers::Equals(std::vector<std::string_view>{"awdawd", "wadawdaw"}));
        }
        SECTION("--compile alternative")
        {
            std::vector<std::string_view> elements = {"awdawd", "--compile", "wadawdaw"};
            auto cli = coral66::parseCommandLine<COMPILE_ONLY>(elements);
            CHECK(cli.get<COMPILE_ONLY>());
            CHECK_THAT(cli.getUnrecognized(),
                       Catch::Matchers::Equals(std::vector<std::string_view>{"awdawd", "wadawdaw"}));
        }
    }
    SECTION("Appears 0 times")
    {
        std::vector<std::string_view> elements = {"awdawd", "wadawdaw"};
        auto cli = coral66::parseCommandLine<COMPILE_ONLY>(elements);
        CHECK_FALSE(cli.get<COMPILE_ONLY>());
        CHECK_THAT(cli.getUnrecognized(), Catch::Matchers::Equals(std::vector<std::string_view>{"awdawd", "wadawdaw"}));
    }
    SECTION("Appears multiple times")
    {
        std::vector<std::string_view> elements = {"awdawd", "-c", "wadawdaw", "-c", "--compile"};
        auto cli = coral66::parseCommandLine<COMPILE_ONLY>(elements);
        CHECK(cli.get<COMPILE_ONLY>());
        CHECK_THAT(cli.getUnrecognized(), Catch::Matchers::Equals(std::vector<std::string_view>{"awdawd", "wadawdaw"}));
    }
}

TEST_CASE("Commandline arg types", "[cli]")
{
    SECTION("Don't match against whitespace")
    {
        std::vector<std::string_view> elements = {"-I", "text"};
        std::string storage;
        llvm::raw_string_ostream ss(storage);
        auto cli = coral66::parseCommandLine<INCLUDES>(elements, &ss);
        CHECK(cli.get<INCLUDES>().empty());
        CHECK_THAT(cli.getUnrecognized(), Catch::Matchers::Equals(std::vector<std::string_view>{"-I", "text"}));
        CHECK_THAT(storage, ProducesError(EXPECTED_ARGUMENT_IMMEDIATELY_AFTER_N, "-I"));
    }
    SECTION("Strings")
    {
        SECTION("std::string")
        {
            std::vector<std::string_view> elements = {"--prefix", "text"};
            auto cli = coral66::parseCommandLine<STRING_ARG>(elements);
            REQUIRE(cli.get<STRING_ARG>());
            CHECK(*cli.get<STRING_ARG>() == "text");
        }
        SECTION("std::string_view")
        {
            std::vector<std::string_view> elements = {"--prefix", "text"};
            auto cli = coral66::parseCommandLine<STRING_VIEW_ARG>(elements);
            REQUIRE(cli.get<STRING_VIEW_ARG>());
            CHECK(*cli.get<STRING_VIEW_ARG>() == "text");
        }
    }
    SECTION("Integral types")
    {
        SECTION("Success")
        {
            std::vector<std::string_view> elements = {"-O3"};
            auto cli = coral66::parseCommandLine<OPT>(elements);
            REQUIRE(cli.get<OPT>());  // check that OPT occured
            REQUIRE(*cli.get<OPT>()); // check that OPT with a value occured
            CHECK(**cli.get<OPT>() == 3);
        }
        SECTION("Failure")
        {
            std::vector<std::string_view> elements = {"-O3t"};
            std::string storage;
            llvm::raw_string_ostream ss(storage);
            auto cli = coral66::parseCommandLine<OPT>(elements, &ss);
            CHECK_FALSE(cli.get<OPT>());
            CHECK_THAT(storage, ProducesError(ERRORS_PARSING_INTEGER_ARGUMENT_IN_N, "-O3t"));
        }
    }
    SECTION("Char")
    {
        SECTION("Success")
        {
            std::vector<std::string_view> elements = {"-O〺"};
            auto cli = coral66::parseCommandLine<OPT_CHAR>(elements);
            REQUIRE(cli.get<OPT_CHAR>());
            REQUIRE(*cli.get<OPT_CHAR>());
            CHECK(static_cast<std::uint32_t>(**cli.get<OPT_CHAR>()) == U'〺');
        }
        SECTION("Failure")
        {
            std::vector<std::string_view> elements = {"-O\xE0\x80\x80"};
            std::string storage;
            llvm::raw_string_ostream ss(storage);
            auto cli = coral66::parseCommandLine<OPT_CHAR>(elements, &ss);
            CHECK_FALSE(cli.get<OPT_CHAR>());
            CHECK_THAT(storage, ProducesError(ERRORS_PARSING_INVALID_UTF8_IN_N, "-O\xE0\x80\x80"));
        }
    }
}

TEST_CASE("Commandline with multi arg", "[cli]")
{
    SECTION("List")
    {
        SECTION("Appears 0 times")
        {
            std::vector<std::string_view> elements = {"awdawd", "wadawdaw", "test"};
            auto cli = coral66::parseCommandLine<INCLUDES>(elements);
            CHECK_THAT(cli.get<INCLUDES>(), Catch::Matchers::Equals(std::vector<std::string_view>{}));
        }
        SECTION("Appears multiple times")
        {
            std::vector<std::string_view> elements = {"-Iawdawd", "--include-directory", "wadawdaw",
                                                      "--include-directory=test"};
            auto cli = coral66::parseCommandLine<INCLUDES>(elements);
            CHECK_THAT(cli.get<INCLUDES>(),
                       Catch::Matchers::Equals(std::vector<std::string_view>{"awdawd", "wadawdaw", "test"}));
        }
    }
}

TEST_CASE("Commandline negate", "[cli]")
{
    std::vector<std::string_view> elements = {"awdawd", "wadawdaw", "test"};
    auto cli = coral66::parseCommandLine<PIE>(elements);
    CHECK_FALSE(cli.get<PIE>());
    elements = {"awdawd", "-fpie", "test"};
    cli = coral66::parseCommandLine<PIE>(elements);
    CHECK(cli.get<PIE>());
    elements = {"-fpie", "-fpie", "-fpie"};
    cli = coral66::parseCommandLine<PIE>(elements);
    CHECK(cli.get<PIE>());
    elements = {"-fpie", "-fpie", "-fno-pie"};
    cli = coral66::parseCommandLine<PIE>(elements);
    CHECK_FALSE(cli.get<PIE>());
    elements = {"-fpie", "-fno-pie", "-fpie"};
    cli = coral66::parseCommandLine<PIE>(elements);
    CHECK(cli.get<PIE>());
}
