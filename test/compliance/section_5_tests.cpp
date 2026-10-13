/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 13/10/2026
 */

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>

#include <iostream>

#include "lib/backend/code_generation.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/lexer/lexer.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "lib/frontend/semantics/program.hpp"
#include "lib/frontend/semantics/semantic_support.hpp"
#include "llvm/IR/DataLayout.h"
#include "test/common/test_config.hpp"
#include "test/compliance/common/common.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace Catch::Matchers;
using namespace coral66::errors::semantics;

///
/// Section 5
///
TEST_CASE("Section 5", "[Compliance][Section-5]")
{
    /// Place references refer to positions of program statements, and the simplest
    /// position marker is the label (section 3.4). A switch is a preset and unalterable
    /// array of labels, which must be within scope at the switch declaration. Any use
    /// of the indexed switch name refers to the corresponding label.
    SECTION("5-1")
    {
        auto sourcePath = getTestDataPath("section-5/5-1.cor", "COMPLIANCE_DATA_DIR");
        auto source = readFile(sourcePath);
        CHECK_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        // Frontend passes
        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        CHECK_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        CHECK_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        CHECK_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        CHECK_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);

        // Switch array is unalterable
        CHECK(errors);
        CHECK(coral66::semantics::DiagnosticCollector::getDiagnostics().size() == 1);
        CHECK(coral66::semantics::DiagnosticCollector::findDiagnostic(CANNOT_ASSIGN_TO_SWITCH));
    }
}
