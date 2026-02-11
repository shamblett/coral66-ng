/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/main/command_line_arguments.hpp"
#include "lib/main/command_line_processor.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;

TEST_CASE("Initial state", "[cli]")
{
    CHECK(OutputFilename.empty());
    CHECK(CompileOnly == false);
    CHECK(CompileAndAssembly == false);
    CHECK(PreprocessOutput == false);
    CHECK(EmitLLVM == false);
    CHECK(OptimizationLevel == OptLevel::None);
    CHECK(LibraryDirectory.empty());
    CHECK(Pie == false);
    CHECK(Pic == false);
    CHECK(Version == false);
    CHECK(DebugLevel == DebugLevel::NoDebug);
    CHECK(SyntaxOnly == false);
    CHECK(Time == false);
    CHECK(SourceFilename.empty());
}
TEST_CASE("Commandline without args", "[cli]")
{
    auto [argc, argv] = createArgs("");
    CommandLineProcessor::ParseCommandLine(argc, argv);
    CHECK(SourceFilename.empty());
}

TEST_CASE("Commandline with just source file name", "[cli]")
{
    auto [argc, argv] = createArgs("main.cor");
    CommandLineProcessor::ParseCommandLine(argc, argv);
    CHECK(SourceFilename == "main.cor");
}

TEST_CASE("Commandline options", "[cli]")
{
    SECTION("Compile")
    {
        auto [argc, argv] = createArgs("-c main.cor");
        CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(CompileOnly == true);
    }

    SECTION("Compile and Assemble")
    {
        auto [argc, argv] = createArgs("-s main.cor");
        CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(CompileAndAssembly == true);
    }

    SECTION("Preprocess output")
    {
        auto [argc, argv] = createArgs("-e main.cor");
        CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(PreprocessOutput == true);
    }

    SECTION("Emit LLVM")
    {
        auto [argc, argv] = createArgs("-emit-llvm main.cor");
        CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(EmitLLVM == true);
    }

    SECTION("PIC")
    {
        auto [argc, argv] = createArgs("-pic main.cor");
        CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(Pic == true);
    }

    SECTION("PIE")
    {
        auto [argc, argv] = createArgs("-pie main.cor");
        CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(Pie == true);
    }

    SECTION("Syntax Only")
    {
        auto [argc, argv] = createArgs("-syntax-only main.cor");
        CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(SyntaxOnly == true);
    }

    SECTION("Time")
    {
        auto [argc, argv] = createArgs("-time main.cor");
        CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(Time == true);
    }
}
