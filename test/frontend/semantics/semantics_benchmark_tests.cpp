/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 07/05/2026
 */

#include <iostream>

#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "lib/frontend/semantics/semantics.hpp"
#include "lib/frontend/semantics/semantic_support.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::syntax;
using namespace coral66::Tests;

/*
 * Benchmark tests
 */

TEST_CASE("Benchmark", "[Semantics][Benchmark]")
{
    SECTION("Ackermann")
    {
        auto sourcePath = getTestDataPath("benchmark/ackerman.cor", "frontend/semantics");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        semantics::DiagnosticCollector::clearDiagnostics();
        auto parsed = parser::parseProgram(cTokens, &ss);

    }

    SECTION("Sieve")
    {
        auto sourcePath = getTestDataPath("benchmark/sieve.cor", "frontend/semantics");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        semantics::DiagnosticCollector::clearDiagnostics();
        auto parsed = parser::parseProgram(cTokens, &ss);

    }
}
