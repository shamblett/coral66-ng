/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 02/06/2026
 */

#include <iostream>

#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "lib/frontend/semantics/program.hpp"
#include "test/common/test_config.hpp"
#include "test/frontend/common/common.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"
#include "tools/semantics/programdump/lib/program_dump.hpp"

using namespace coral66;
using namespace coral66::syntax;
using namespace coral66::Tests;

/*
 * Golden tests
 *
 * Note that until the duplicate AST node problem outlined in the random-ordering.md document
 * is fixed we can only compare the source and genertaed on size, which of course should always be
 * the same.
 */

TEST_CASE("Examples", "[Semantics][Golden]")
{
    SECTION("Ackermann")
    {
        auto sourcePath = getTestDataPath("ackermann/ackerman.cor", "GOLDEN_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        auto parsed = parser::parseProgram(cTokens, &ss);
        bool errors = false;
        auto program = semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        auto sourceProgram = tools::ProgramDump::dumpToString(program, reporterString);
        auto programPath = getTestDataPath("ackermann/ackerman_program.json", "GOLDEN_DIR");
        auto generatedProgram = readFile(programPath);
        CHECK(!generatedProgram.empty());

        CHECK(sourceProgram.size() == generatedProgram.size());
    }

    SECTION("Sieve") {}
}

TEST_CASE("Generated", "[Parser][Golden]")
{
    SECTION("G1") {}
}