/*
* Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "test/vendor/catch2v3/catch_amalgamated.hpp"

#include "lib/frontend/common/ErrorMessages.hpp"
#include "lib/main/Main.hpp"

#include "test/common/TestConfig.hpp"

#define coral66_MAIN_PRODUCES(matcher, ...)       \
    [](std::vector<std::string_view> ref)     \
    {                                         \
        std::string storage;                  \
        llvm::raw_string_ostream ss(storage); \
        coral66::main(ref, &ss);                  \
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
    SECTION("Compile")
    {
        auto dummyFile = createInclude("main.cor", "PROGRAM LoopTest\n"
            "BEGIN\n"
            "  INTEGER i;\n"
            "  FOR i := 0 STEP 2 UNTIL 10 DO\n"
            "    ANSWER i;\n"
            "END");
        SECTION("Normal")
        {
            std::vector<std::string_view> vector = {"-c", "main.cor"};
            auto ret = coral66::main(vector);
            CHECK(ret == 0);
            CHECK(coral66::fs::exists(coral66::fs::path("main.o")));
        }
        SECTION("LLVM")
        {
            std::vector<std::string_view> vector = {"-c", "main.cor", "-emit-llvm"};
            auto ret = coral66::main(vector);
            CHECK(ret == 0);
            CHECK(coral66::fs::exists(coral66::fs::path("main.bc")));
        }
        coral66_MAIN_PRODUCES(ProducesError(CANNOT_COMPILE_TO_OBJECT_FILE_AND_ASSEMBLY_AT_THE_SAME_TIME),
                          {"-c", "main.cor", "-S"});
        coral66_MAIN_PRODUCES(ProducesError(CANNOT_COMPILE_TO_OBJECT_FILE_AND_PREPROCESS_AT_THE_SAME_TIME),
                          {"-c", "main.cor", "-E"});
    }
    SECTION("Assembly")
    {
        auto dummyFile = createInclude("main.cor", "CORAL TEST RESPONDER INTEGER A END");
        SECTION("Normal")
        {
            std::vector<std::string_view> vector = {"-S", "main.cor"};
            auto ret = coral66::main(vector);
            CHECK(ret == 0);
            CHECK(coral66::fs::exists(coral66::fs::path("main.s")));
        }
        SECTION("LLVM")
        {
            std::vector<std::string_view> vector = {"-S", "main.cor", "-emit-llvm"};
            auto ret = coral66::main(vector);
            CHECK(ret == 0);
            CHECK(coral66::fs::exists(coral66::fs::path("main.ll")));
        }
        coral66_MAIN_PRODUCES(ProducesError(CANNOT_COMPILE_TO_ASSEMBLY_AND_PREPROCESS_AT_THE_SAME_TIME),
                          {"-S", "main.cor", "-E"});
    }

    SECTION("output file")
    {
        auto dummyFile = createInclude("main.cor", "CORAL TEST RESPONDER INTEGER A END");
        coral66::fs::remove(coral66::fs::path("test.o"));
        std::vector<std::string_view> vector = {"-c", "main.cor", "-o", "test.o"};
        CHECK(coral66::main(vector) == 0);
        CHECK(coral66::fs::exists(coral66::fs::path("test.o")));
    }
    // SECTION("disabling warnings")
    // {
    //     auto dummyFile = createInclude("main.c", "#define foo 0\n"
    //                                              "#define foo 0\n"
    //                                              "int main(void) { return foo; }");
    //     coral66_MAIN_PRODUCES(ProducesNoWarnings(), {"-c", "main.c", "-Wno-macro-redefined"});
    // }
    SECTION("Version")
    {
        std::vector<std::string_view> vector = {"--version"};
        std::string s;
        llvm::raw_string_ostream ss(s);
        auto ret = coral66::main(vector, nullptr, &ss);
        CHECK(ret == 0);
        ss.flush();
        CHECK_THAT(s, Catch::Matchers::Matches("coral66 version \\d+\\.\\d+\\.\\d+(.|\\n)*"));
    }
    auto dummyFile = createInclude("main.cor", "CORAL TEST RESPONDER INTEGER A END");
    coral66_MAIN_PRODUCES(ProducesError(UNKNOWN_LANGUAGE_STANDARD_N, "java8"), {"-std=java8", "-c", "main.c"});
    coral66_MAIN_PRODUCES(ProducesError(NO_SOURCE_FILES_SPECIFIED), {"-c"});
    coral66_MAIN_PRODUCES(ProducesError(FAILED_TO_OPEN_C_SOURCE_FILE_N, "missing.c"), {"missing.c", "-c"});
    coral66_MAIN_PRODUCES(ProducesError(FAILED_TO_OPEN_FILE_N_FOR_OUTPUT, "/PleaseDoNotExitThatWouldBeSuperAwkward/main.o"),
                      {"main.cor", "-c", "-o", "/PleaseDoNotExitThatWouldBeSuperAwkward/main.o"});
}
