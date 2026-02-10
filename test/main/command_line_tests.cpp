/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include <wordexp.h>
#include <print>

#include "lib/main/command_line_arguments.hpp"
#include "lib/main/command_line_processor.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

std::tuple<int, char**> createArgs(const std::string& commandLine)
{
    wordexp_t p;
    p.we_offs = 1;
    wordexp(commandLine.c_str(), &p, WRDE_DOOFFS);
    return std::make_tuple(p.we_wordc, p.we_wordv);
}

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
    auto [argc, argv] = createArgs("main.cor");
    CommandLineProcessor::ParseCommandLine(argc, argv);
    print(SourceFilename);
    CHECK(SourceFilename == "main.cor");
}

TEST_CASE("Commandline arg types", "[cli]")
{

}

