/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/main/main.hpp"

#include <iostream>
#include <print>

#include "lib/frontend/common/errors.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66::Errors::CLI;

TEST_CASE("Commandline actions", "[cli]")
{

    std::string buffer;
    llvm::raw_string_ostream reporter(buffer);

    SECTION("Compile no source file")
    {
        auto [argc, argv] = createArgs("-c");
        CHECK(coral66::main(argc, argv, &reporter) == EXIT_FAILURE);
        reporter.flush();
        CHECK(buffer == "coral66: error: No source files specified\n");
    }

    SECTION("Compile wrong source file extension")
    {
        auto [argc, argv] = createArgs("-c main.cpp");
        CHECK(coral66::main(argc, argv, &reporter) == EXIT_FAILURE);
        reporter.flush();
        CHECK(buffer == "coral66: error: The source file extension is not .cor 'main.cpp'\n");
    }

}
