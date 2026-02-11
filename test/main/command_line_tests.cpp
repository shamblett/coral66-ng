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
    auto [argc, argv] = createArgs("main.cor");
    CommandLineProcessor::ParseCommandLine(argc, argv);
    CHECK(SourceFilename == "main.cor");
}

TEST_CASE("Commandline arg types", "[cli]")
{

}

