/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/main/main.hpp"

#include <iostream>

#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/messages/warnings.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66::errors::CLI;
using namespace coral66::warnings::CLI;

TEST_CASE("Commandline actions", "[cli]")
{
    std::string buffer;
    llvm::raw_string_ostream reporter(buffer);

    SECTION("Compile no source file")
    {
        auto [argc, argv] = createArgs("-c");
        CHECK(coral66::main(argc, argv, &reporter) == EXIT_FAILURE);
        reporter.flush();
        CHECK(ProducesError(NO_SOURCE_FILES_SPECIFIED).match(buffer));
    }

    SECTION("Compile wrong source file extension")
    {
        auto [argc, argv] = createArgs("-c main.cpp");
        CHECK(coral66::main(argc, argv, &reporter) == EXIT_FAILURE);
        reporter.flush();
        auto errorMessage = SOURCE_FILE_EXTENSION_NOT_COR.argsCLI(std::string("main.cpp"));
        CHECK(buffer == errorMessage.getText());
    }

    SECTION("No Action")
    {
        auto [argc, argv] = createArgs("-noact -c main.cor");
        CHECK(coral66::main(argc, argv, &reporter) == EXIT_SUCCESS);
        reporter.flush();
        CHECK(ProducesWarning(NO_ACTION_SPEFIFIED).match(buffer));
    }

    SECTION("Compile to object file and assembly")
    {
        auto [argc, argv] = createArgs("-c -s -o out.o -noact main.cor");
        CHECK(coral66::main(argc, argv, &reporter) == EXIT_SUCCESS);
        reporter.flush();
        CHECK(ProducesWarning(CANNOT_COMPILE_TO_OBJECT_FILE_AND_ASSEMBLY_AT_THE_SAME_TIME).match(buffer));
    }

    SECTION("Compile to object file and preprocessor")
    {
        auto [argc, argv] = createArgs("-c -e -noact main.cor");
        CHECK(coral66::main(argc, argv, &reporter) == EXIT_SUCCESS);
        reporter.flush();
        CHECK(ProducesWarning(CANNOT_COMPILE_TO_OBJECT_FILE_AND_PREPROCESS_AT_THE_SAME_TIME).match(buffer));
    }

    SECTION("Assemble and preprocess")
    {
        auto [argc, argv] = createArgs("-s -e -noact main.cor");
        CHECK(coral66::main(argc, argv, &reporter) == EXIT_SUCCESS);
        reporter.flush();
        CHECK(ProducesWarning(CANNOT_COMPILE_TO_ASSEMBLY_AND_PREPROCESS_AT_THE_SAME_TIME).match(buffer));
    }

    SECTION("Failed to open output file")
    {
        auto [argc, argv] = createArgs("-c " + getTestDataPath("main.cor", "main") + " -o /nosuch/dir/billy");
        CHECK(coral66::main(argc, argv, &reporter) == EXIT_FAILURE);
        reporter.flush();
        auto errorMessage = FAILED_TO_OPEN_FILE_FOR_OUTPUT.argsCLI(std::string("/nosuch/dir/billy"));
        CHECK(buffer == errorMessage.getText());
    }
}
