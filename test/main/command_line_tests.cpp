/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/main/command_line_arguments.hpp"
#include "lib/main/command_line_processor.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66::cl;

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
    CHECK(NoAction == false);
    CHECK(SourceFilename.empty());
}
TEST_CASE("Commandline without args", "[cli]")
{
    auto [argc, argv] = createArgs("");
    coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
    CHECK(SourceFilename.empty());
}

TEST_CASE("Commandline with just source file name", "[cli]")
{
    auto [argc, argv] = createArgs("main.cor");
    coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
    CHECK(SourceFilename == "main.cor");
}

TEST_CASE("Boolean options", "[cli]")
{
    SECTION("Compile")
    {
        auto [argc, argv] = createArgs("-c main.cor");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(CompileOnly == true);
    }

    SECTION("Compile and Assemble")
    {
        auto [argc, argv] = createArgs("-s main.cor");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(CompileAndAssembly == true);
    }

    SECTION("Preprocess output")
    {
        auto [argc, argv] = createArgs("-e main.cor");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(PreprocessOutput == true);
    }

    SECTION("Emit LLVM")
    {
        auto [argc, argv] = createArgs("-emit-llvm main.cor");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(EmitLLVM == true);
    }

    SECTION("PIC")
    {
        auto [argc, argv] = createArgs("-pic main.cor");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(Pic == true);
    }

    SECTION("PIE")
    {
        auto [argc, argv] = createArgs("-pie main.cor");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(Pie == true);
    }

    SECTION("Syntax Only")
    {
        auto [argc, argv] = createArgs("-syntax-only main.cor");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(SyntaxOnly == true);
    }

    SECTION("Parse Only")
    {
        auto [argc, argv] = createArgs("-parse-only main.cor");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(ParseOnly == true);
    }

    SECTION("Time")
    {
        auto [argc, argv] = createArgs("-time main.cor");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(Time == true);
    }

    SECTION("No Action")
    {
        auto [argc, argv] = createArgs("-c -noact main.cor");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(NoAction == true);
    }
}

TEST_CASE("Alternative options", "[cli]")
{
    SECTION("Optimisation level")
    {
        auto [argc, argv] = createArgs("-O2 main.cor");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(OptimizationLevel == OptLevel::O2);
    }

    SECTION("Optimisation level - multi")
    {
        auto [argc, argv] = createArgs("-O2 -O3 -O1 main.cor");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(OptimizationLevel == OptLevel::O1);
    }

    SECTION("Debug level")
    {
        auto [argc, argv] = createArgs("-g2 main.cor");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(DebugLevel == DebugLevel::g2);
    }

    SECTION("Debug level - multi")
    {
        auto [argc, argv] = createArgs("-g2 -g1 -g3 main.cor");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(DebugLevel == DebugLevel::g3);
    }
}

TEST_CASE("Version", "[cli]")
{
    std::string str;
    llvm::raw_string_ostream os(str);
    coral66::CommandLineProcessor::PrintVersion(os);
    CHECK(os.str() == "1.0-frontend\n");
}

TEST_CASE("String options")
{
    SECTION("Output file")
    {
        auto [argc, argv] = createArgs("-o main.o");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(OutputFilename == "main.o");
    }

    SECTION("Library directory")
    {
        auto [argc, argv] = createArgs("-L path/to/libs");
        coral66::CommandLineProcessor::ParseCommandLine(argc, argv);
        CHECK(LibraryDirectory == "path/to/libs");
    }
}
