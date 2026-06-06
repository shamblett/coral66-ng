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
 * is fixed we can only compare the source and generated on size, which of course should always be
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

    SECTION("Sieve")
    {
        auto sourcePath = getTestDataPath("sieve/sieve.cor", "GOLDEN_DIR");
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
        auto programPath = getTestDataPath("sieve/sieve_program.json", "GOLDEN_DIR");
        auto generatedProgram = readFile(programPath);
        CHECK(!generatedProgram.empty());

        CHECK(sourceProgram.size() == generatedProgram.size());
    }
}

TEST_CASE("Generated", "[Semantics][Golden]")
{
    SECTION("G1")
    {
        auto sourcePath = getTestDataPath("generated/g1.cor", "GOLDEN_DIR");
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
        auto programPath = getTestDataPath("generated/g1_program.json", "GOLDEN_DIR");
        auto generatedProgram = readFile(programPath);
        CHECK(!generatedProgram.empty());

        CHECK(sourceProgram.size() == generatedProgram.size());
    }

    SECTION("G2")
    {
        auto sourcePath = getTestDataPath("generated/g2.cor", "GOLDEN_DIR");
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
        auto programPath = getTestDataPath("generated/g2_program.json", "GOLDEN_DIR");
        auto generatedProgram = readFile(programPath);
        CHECK(!generatedProgram.empty());

        CHECK(sourceProgram.size() == generatedProgram.size());
    }

    SECTION("G3")
    {
        auto sourcePath = getTestDataPath("generated/g3.cor", "GOLDEN_DIR");
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
        auto programPath = getTestDataPath("generated/g3_program.json", "GOLDEN_DIR");
        auto generatedProgram = readFile(programPath);
        CHECK(!generatedProgram.empty());

        CHECK(sourceProgram.size() == generatedProgram.size());
    }

    SECTION("G4")
    {
        auto sourcePath = getTestDataPath("generated/g4.cor", "GOLDEN_DIR");
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
        auto programPath = getTestDataPath("generated/g4_program.json", "GOLDEN_DIR");
        auto generatedProgram = readFile(programPath);
        CHECK(!generatedProgram.empty());

        CHECK(sourceProgram.size() == generatedProgram.size());
    }

    SECTION("G5")
    {
        auto sourcePath = getTestDataPath("generated/g5.cor", "GOLDEN_DIR");
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
        auto programPath = getTestDataPath("generated/g5_program.json", "GOLDEN_DIR");
        auto generatedProgram = readFile(programPath);
        CHECK(!generatedProgram.empty());

        CHECK(sourceProgram.size() == generatedProgram.size());
    }

    SECTION("G6")
    {
        auto sourcePath = getTestDataPath("generated/g6.cor", "GOLDEN_DIR");
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
        auto programPath = getTestDataPath("generated/g6_program.json", "GOLDEN_DIR");
        auto generatedProgram = readFile(programPath);
        CHECK(!generatedProgram.empty());

        CHECK(sourceProgram.size() == generatedProgram.size());
    }

    SECTION("G7")
    {
        auto sourcePath = getTestDataPath("generated/g7.cor", "GOLDEN_DIR");
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
        auto programPath = getTestDataPath("generated/g7_program.json", "GOLDEN_DIR");
        auto generatedProgram = readFile(programPath);
        CHECK(!generatedProgram.empty());

        CHECK(sourceProgram.size() == generatedProgram.size());
    }

    SECTION("G8")
    {
        auto sourcePath = getTestDataPath("generated/g8.cor", "GOLDEN_DIR");
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
        auto programPath = getTestDataPath("generated/g8_program.json", "GOLDEN_DIR");
        auto generatedProgram = readFile(programPath);
        CHECK(!generatedProgram.empty());

        CHECK(sourceProgram.size() == generatedProgram.size());
    }

    SECTION("G9")
    {
        auto sourcePath = getTestDataPath("generated/g9.cor", "GOLDEN_DIR");
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
        auto programPath = getTestDataPath("generated/g9_program.json", "GOLDEN_DIR");
        auto generatedProgram = readFile(programPath);
        CHECK(!generatedProgram.empty());

        CHECK(sourceProgram.size() == generatedProgram.size());
    }

    SECTION("G10")
    {
        auto sourcePath = getTestDataPath("generated/g10.cor", "GOLDEN_DIR");
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
        auto programPath = getTestDataPath("generated/g10_program.json", "GOLDEN_DIR");
        auto generatedProgram = readFile(programPath);
        CHECK(!generatedProgram.empty());

        CHECK(sourceProgram.size() == generatedProgram.size());
    }

    SECTION("G11")
    {
        auto sourcePath = getTestDataPath("generated/g11.cor", "GOLDEN_DIR");
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
        auto programPath = getTestDataPath("generated/g11_program.json", "GOLDEN_DIR");
        auto generatedProgram = readFile(programPath);
        CHECK(!generatedProgram.empty());

        CHECK(sourceProgram.size() == generatedProgram.size());
    }

    SECTION("G12")
    {
        auto sourcePath = getTestDataPath("generated/g12.cor", "GOLDEN_DIR");
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
        auto programPath = getTestDataPath("generated/g12_program.json", "GOLDEN_DIR");
        auto generatedProgram = readFile(programPath);
        CHECK(!generatedProgram.empty());

        CHECK(sourceProgram.size() == generatedProgram.size());
    }
}