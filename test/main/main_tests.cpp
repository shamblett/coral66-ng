/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/main/main.hpp"

#include <unistd.h>
#include <pwd.h>
#include <print>

#include "lib/frontend/common/error_messages.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

#define coral66_MAIN_PRODUCES(matcher, ...)   \
    [](std::vector<std::string_view> ref)     \
    {                                         \
        std::string storage;                  \
        llvm::raw_string_ostream ss(storage); \
        coral66::main(ref, &ss);              \
        ss.flush();                           \
        CHECK_THAT(storage, matcher);         \
        if (!storage.empty())                 \
        {                                     \
            llvm::errs() << storage << '\n';  \
        }                                     \
    }(__VA_ARGS__)

using namespace coral66::Errors::CLI;

TEST_CASE("Commandline options", "[coral66]")
{
    SECTION("Version")
    {
        std::vector<std::string_view> vector = {"--version"};
        std::string s;
        llvm::raw_string_ostream ss(s);
        const auto ret = coral66::main(vector, nullptr, &ss);
        CHECK(ret == 0);
        ss.flush();
        REQUIRE(s == "coral66 version 1.0-beta\n");
    }

    SECTION("Compile")
    {
        const passwd *pw = getpwuid(getuid());
        const char *homedir = pw->pw_dir;
        const std::string homeDir = std::string(homedir);
        std::println("Home: {}", homeDir);
        const std::string mainFile = homeDir + "/Development/c++/coral/coral66-ng/test/main/data/main/main.cor";
        std::println("Name: {}", mainFile);
        coral66_MAIN_PRODUCES(ProducesError(NO_SOURCE_FILES_SPECIFIED), {"-c"});
        coral66_MAIN_PRODUCES(ProducesError(FAILED_TO_OPEN_CORAL66_SOURCE_FILE_N, "missing.cor"), {"missing.cor", "-c"});
        coral66_MAIN_PRODUCES(ProducesError(FAILED_TO_OPEN_FILE_N_FOR_OUTPUT, "/PleaseDoNotExitThatWouldBeSuperAwkward/main.o"),
            {mainFile, "-c", "-o", "/PleaseDoNotExitThatWouldBeSuperAwkward/main.o"});
    }

}
