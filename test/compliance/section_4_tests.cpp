/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 21/07/2026
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
/// Section 4
///
TEST_CASE("Section 4", "[Compliance][Section-4]")
{
    /// There are three types of number, floating-point, fixed-point and integer.
    /// Except in certain part-word table-elements (section 4.4.2.2), all three types
    /// are signed. Numeric type is indicated by the word FLOATING or INTEGER,
    /// or by the word FIXED followed by scaling constants which must be given
    /// numerically.
    /// It is assumedthroughout this definition that a number is confined within a
    /// single computer word which in this implementation is 32 bits long.
    SECTION("4-1-1")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-4/4-1-1.cor", "COMPLIANCE_DATA_DIR");
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
        CHECK_FALSE(errors);

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("fouroneone", context);
        REQUIRE(module.get() != nullptr); // Needs to be REQUIRE

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        CHECK_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr); // Needs to be REQUIRE

        // Run the `main` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr); // Needs to be REQUIRE

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        bool checkInteger = false;
        bool checkFloating = false;
        bool checkFixed = false;

        // Integer
        if (const llvm::GlobalVariable* gVal = ee->FindGlobalVariableNamed("theInteger"))
        {
            const llvm::DataLayout& dl = ee->getDataLayout();
            uint64_t sizeInBytes = dl.getTypeAllocSize(gVal->getValueType()).getFixedValue();
            CHECK(sizeInBytes == 4);
            checkInteger = true;
        }

        // Floating
        if (const llvm::GlobalVariable* gVal = ee->FindGlobalVariableNamed("theFloat"))
        {
            const llvm::DataLayout& dl = ee->getDataLayout();
            uint64_t sizeInBytes = dl.getTypeAllocSize(gVal->getValueType()).getFixedValue();
            CHECK(sizeInBytes == 4);
            checkFloating = true;
        }

        // Fixed
        if (const llvm::GlobalVariable* gVal = ee->FindGlobalVariableNamed("theFixed"))
        {
            const llvm::DataLayout& dl = ee->getDataLayout();
            uint64_t sizeInBytes = dl.getTypeAllocSize(gVal->getValueType()).getFixedValue();
            CHECK(sizeInBytes == 4);
            checkFixed = true;
        }

        CHECK(checkInteger);
        CHECK(checkFloating);
        CHECK(checkFixed);

        // Clean up
        delete ee;
    }

    /// A fixed Numeric type is indicated by the word FIXED followed by scaling
    /// constants which must be given numerically, e.g.

    /// FIXED (13,5)

    /// This specifies five fractional bits and a minimum of 13 bits to represent the
    /// number as a whole, including the fractional bits and a sign.
    SECTION("4-1-2")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-4/4-1-2.cor", "COMPLIANCE_DATA_DIR");
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
        CHECK_FALSE(errors);

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("fouronetwo", context);
        REQUIRE(module.get() != nullptr); // Needs to be REQUIRE

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        CHECK_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr); // Needs to be REQUIRE

        // Run the `main` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr); // Needs to be REQUIRE

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // f1351
        auto fixedAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("f1351"));
        REQUIRE(fixedAddress != nullptr);
        // Dereference to get the actual value
        int32_t fixed = *static_cast<int32_t*>(fixedAddress);
        /// 8.75 * 32 = 280
        CHECK(fixed == 280);

        // f1352
        fixedAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("f1352"));
        REQUIRE(fixedAddress != nullptr);
        // Dereference to get the actual value
        fixed = *static_cast<int32_t*>(fixedAddress);
        /// 87.5 * 32 = 2800
        CHECK(fixed == 2800);

        // f1353
        fixedAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("f1353"));
        REQUIRE(fixedAddress != nullptr);
        // Dereference to get the actual value
        fixed = *static_cast<int32_t*>(fixedAddress);
        /// 875 * 32 = 28000
        CHECK(fixed == 28000);

        // f1354 - same as f1353 but RHS is an integer not a float
        fixedAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("f1354"));
        REQUIRE(fixedAddress != nullptr);
        // Dereference to get the actual value
        fixed = *static_cast<int32_t*>(fixedAddress);
        /// 875 * 32 = 28000
        CHECK(fixed == 28000);
        // Clean up
        delete ee;
    }

    /// An array is restricted to a one or two dimensional set of numbers all of the
    /// same type (including scale for fixed-point).
    SECTION("4-3-1")
    {
        auto sourcePath = getTestDataPath("section-4/4-3-1.cor", "COMPLIANCE_DATA_DIR");
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
        CHECK(errors);

        CHECK(reporterString.find("Type mismatch in expression") != std::string::npos);
        CHECK(reporterString.find("b[0] :=") == std::string::npos); // No type mismatch
        CHECK(reporterString.find("b[3] :=") != std::string::npos); // Type mismatch
    }

    /// An array lower bound must never exceed the corresponding upper bound.
    SECTION("4-3-2")
    {
        auto sourcePath = getTestDataPath("section-4/4-3-2.cor", "COMPLIANCE_DATA_DIR");
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
        CHECK(errors);

        CHECK(coral66::semantics::DiagnosticCollector::getDiagnostics().size() == 1);
        CHECK(coral66::semantics::DiagnosticCollector::findDiagnostic(INVALID_ARRAY_BOUNDS));
    }

    /// If more than one array is required with the same numeric type, and the same
    /// dimensions and bounds, a list of array identifiers separated by commas may
    /// replace the single identifiers. Arrays with the
    /// same numeric type but different bounds or dimensions may also be included
    /// in a composite declaration.
    SECTION("4-3-3")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-4/4-3-3.cor", "COMPLIANCE_DATA_DIR");
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
        CHECK_FALSE(errors);

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("fourthreethree", context);
        REQUIRE(module.get() != nullptr); // Needs to be REQUIRE

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        CHECK_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr); // Needs to be REQUIRE

        // Run the `main` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr); // Needs to be REQUIRE

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // p[1]
        // 1. Get the base address of the array (points to the lower bound element)
        auto arrayBaseAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("p"));
        REQUIRE(arrayBaseAddress != nullptr);

        // 2. Cast the address to an integer pointer (since it's an INTEGER ARRAY)
        auto elements = static_cast<int32_t*>(arrayBaseAddress);

        // 3. Access the element by applying the zero-indexed offset (index - lower_bound)
        //    e.g. for b[6] where lower bound is 1, the offset is 6 - 1 = 5
        int32_t value = elements[0];
        CHECK(value == 10);

        // q[1]
        arrayBaseAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("q"));
        REQUIRE(arrayBaseAddress != nullptr);

        elements = static_cast<int32_t*>(arrayBaseAddress);

        value = elements[0];
        CHECK(value == 20);

        // r[1]
        arrayBaseAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("r"));
        REQUIRE(arrayBaseAddress != nullptr);

        elements = static_cast<int32_t*>(arrayBaseAddress);

        value = elements[0];
        CHECK(value == 30);

        // s[2]
        arrayBaseAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("s"));
        REQUIRE(arrayBaseAddress != nullptr);

        elements = static_cast<int32_t*>(arrayBaseAddress);

        value = elements[1];
        CHECK(value == 40);

        // u[2,3]
        arrayBaseAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("u"));
        REQUIRE(arrayBaseAddress != nullptr);

        elements = static_cast<int32_t*>(arrayBaseAddress);

        // For multi-dimensional arrays, the Coral 66 LLVM backend uses nested LLVM arrays.
        // The elements are laid out in row-major order.
        // If you want to read u[2, 3]:
        // 1. Calculate the row offset: 2 - 1 = 1
        // 2. Multiply by column dimension size: 1 * 3 = 3
        // 3. Calculate the column offset: 3 - 1 = 2
        // 4. Add them together: 3 + 2 = 5

        value = elements[5];
        CHECK(value == 50);

        // Clean up
        delete ee;
    }

    /// A group of n words is arbitrarily partitioned into bit-fields (with
    /// no fields crossing a word boundary), and the same partitioning is applied to
    /// as many such groups (m say) as are required. The total data-space for a table
    /// is thus nm words.
    SECTION("4-4-1")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-4/4-4-1.cor", "COMPLIANCE_DATA_DIR");
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
        CHECK_FALSE(errors);

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("fourfourone", context);
        REQUIRE(module.get() != nullptr); // Needs to be REQUIRE

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        CHECK_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr); // Needs to be REQUIRE

        // Run the `main` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr); // Needs to be REQUIRE

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // Table
        bool checkOK = false;
        if (const llvm::GlobalVariable* gVal = ee->FindGlobalVariableNamed("april"))
        {
            const llvm::DataLayout& dl = ee->getDataLayout();
            uint64_t sizeInBytes = dl.getTypeAllocSize(gVal->getValueType()).getFixedValue();
            CHECK(sizeInBytes == 3 * 30 * 4);
            checkOK = true;
        }
        CHECK(checkOK);

        // Clean up
        delete ee;
    }

    /// Word-position and bit-position are numbered from zero upwards, and the
    /// least significant digit of a word occupies bit-position zero.
    SECTION("4-4-2")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-4/4-4-2.cor", "COMPLIANCE_DATA_DIR");
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
        CHECK_FALSE(errors);

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("fourfourtwo", context);
        REQUIRE(module.get() != nullptr); // Needs to be REQUIRE

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        CHECK_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr); // Needs to be REQUIRE

        // Run the `main` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr); // Needs to be REQUIRE

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // Weather
        auto tableBaseAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("weather"));
        REQUIRE(tableBaseAddress != nullptr);

        auto elements = static_cast<int32_t*>(tableBaseAddress);

        // Fullday is element 0 from the table base
        CHECK(elements[0] == -559038737); // Signed integer value of 0xDEADBEEF

        // Clean up
        delete ee;
    }

    /// Normally, table elements will be located so that they fall within the declared width of the
    /// table, but a Coral 66 compiler does not check the limits
    SECTION("4-4-3")
    {
        auto sourcePath = getTestDataPath("section-4/4-4-3.cor", "COMPLIANCE_DATA_DIR");
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

        CHECK_FALSE(errors);
        // The table limits are exceeded but there is no diagnostic for this.
        CHECK(coral66::semantics::DiagnosticCollector::getDiagnostics().size() == 0);
    }

    /// An object is not eligible for presetting if it is declared anywhere within
    /// the body of a recursive procedure
    SECTION("4-6-1")
    {
        auto sourcePath = getTestDataPath("section-4/4-6-1.cor", "COMPLIANCE_DATA_DIR");
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

        // Cant preset in a recursive procedure
        CHECK(errors);
        CHECK(coral66::semantics::DiagnosticCollector::getDiagnostics().size() == 1);
        CHECK(coral66::semantics::DiagnosticCollector::findDiagnostic(PRESET_IN_RECURSIVE_PROCEDURE));
    }

    /// An object is not eligible for presetting if it is declared anywhere within
    /// an inner block of the program.
    SECTION("4-6-2")
    {
        auto sourcePath = getTestDataPath("section-4/4-6-2.cor", "COMPLIANCE_DATA_DIR");
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

        // Cant preset in an inner block
        CHECK(errors);
        CHECK(coral66::semantics::DiagnosticCollector::getDiagnostics().size() == 1);
        CHECK(coral66::semantics::DiagnosticCollector::findDiagnostic(PRESET_IN_INNER_BLOCK));
    }

    /// An object is not eligible for presetting if it is declared anywhere within
    /// an inner block of a procedure.
    SECTION("4-6-3")
    {
        auto sourcePath = getTestDataPath("section-4/4-6-3.cor", "COMPLIANCE_DATA_DIR");
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

        // Cant preset in an inner block of a procedure
        CHECK(errors);
        CHECK(coral66::semantics::DiagnosticCollector::getDiagnostics().size() == 1);
        CHECK(coral66::semantics::DiagnosticCollector::findDiagnostic(PRESET_IN_INNER_BLOCK));
    }

    /// The preset constants for variables and arrays are listed at the end of the declaration
    /// after an assignment symbol, and are allocated in the order defined in section 4.5.
    SECTION("4-6-4")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-4/4-6-4.cor", "COMPLIANCE_DATA_DIR");
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
        CHECK_FALSE(errors);
        ss.flush();
        std::cout << reporterString << "\n";

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("foursixfour", context);
        REQUIRE(module.get() != nullptr); // Needs to be REQUIRE

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        CHECK_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr); // Needs to be REQUIRE

        // Run the `main` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr); // Needs to be REQUIRE

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // k array multi dimensional
        auto arrayBaseAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("k"));
        REQUIRE(arrayBaseAddress != nullptr);

        auto elements = static_cast<int32_t*>(arrayBaseAddress);

        // Check the elements from the table base
        CHECK(elements[0] == 11);
        CHECK(elements[1] == 12);
        CHECK(elements[2] == 21);
        CHECK(elements[3] == 22);

        // l array single dimensional
        arrayBaseAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("l"));
        REQUIRE(arrayBaseAddress != nullptr);

        elements = static_cast<int32_t*>(arrayBaseAddress);

        // Check the elements from the table base
        CHECK(elements[0] == 11);
        CHECK(elements[1] == 12);
        CHECK(elements[2] == 13);

        // Integers

        // a
        auto variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("a"));
        REQUIRE(variableAddress != nullptr);
        // Dereference to get the actual value
        int32_t value = *static_cast<int32_t*>(variableAddress);
        REQUIRE(value == 1);

        // b
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("b"));
        REQUIRE(variableAddress != nullptr);
        // Dereference to get the actual value
        value = *static_cast<int32_t*>(variableAddress);
        REQUIRE(value == 2);

        // c
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("c"));
        REQUIRE(variableAddress != nullptr);
        // Dereference to get the actual value
        value = *static_cast<int32_t*>(variableAddress);
        REQUIRE(value == 3);

        // Floating

        // d
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("d"));
        REQUIRE(variableAddress != nullptr);
        // Dereference to get the actual value
        auto fvalue = *static_cast<float*>(variableAddress);
        REQUIRE(fvalue == 2.0);

        // e
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("e"));
        REQUIRE(variableAddress != nullptr);
        // Dereference to get the actual value
        fvalue = *static_cast<float*>(variableAddress);
        REQUIRE(fvalue == 3.0);

        // f
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("f"));
        REQUIRE(variableAddress != nullptr);
        // Dereference to get the actual value
        fvalue = *static_cast<float*>(variableAddress);
        REQUIRE(fvalue == 4.0);

        // Fixed

        // g
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("g"));
        REQUIRE(variableAddress != nullptr);
        int32_t fixed = *static_cast<int32_t*>(variableAddress);
        CHECK(fixed == 280);

        // h
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("h"));
        REQUIRE(variableAddress != nullptr);
        fixed = *static_cast<int32_t*>(variableAddress);
        CHECK(fixed == 2800);

        // i
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("i"));
        REQUIRE(variableAddress != nullptr);
        fixed = *static_cast<int32_t*>(variableAddress);
        CHECK(fixed == 28000);

        // Clean up
        delete ee;
    }

    /// If the internal structure of a table is completely disregarded, the table can be treated as an
    /// ordinary one-dimensional array of whole computer words (4.4.4), and preset as such (4.6.1).
    /// Alternatively, all the table-elements may be preset after their declaration list,
    /// as shown at Elementpresetlist in the syntax of 4.4.1.
    SECTION("4-6-5")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-4/4-6-5.cor", "COMPLIANCE_DATA_DIR");
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
        CHECK_FALSE(errors);
        ss.flush();
        std::cout << reporterString << "\n";

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("foursixfive", context);
        REQUIRE(module.get() != nullptr); // Needs to be REQUIRE

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        CHECK_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr); // Needs to be REQUIRE

        // Run the `main` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr); // Needs to be REQUIRE

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // Table preset as single dimensional array
        auto arrayBaseAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("gears"));
        REQUIRE(arrayBaseAddress != nullptr);

        auto elements = static_cast<int32_t*>(arrayBaseAddress);

        // Check the elements from the table base
        CHECK(elements[0] == 01402371);
        CHECK(elements[1] == 01003162);
        CHECK(elements[2] == 0603655);

        // Table preset using preset list
        arrayBaseAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("gears1"));
        REQUIRE(arrayBaseAddress != nullptr);

        elements = static_cast<int32_t*>(arrayBaseAddress);

        // Check the elements from the table base
        CHECK(elements[0] == 01402371);
        CHECK(elements[1] == 01003162);
        CHECK(elements[2] == 0603655);

        // Clean up
        delete ee;
    }

    /// Overlays. The base may be a simple reference, one-dimensional array reference or a
    /// table reference treated as a one-dimensional array of whole words. If the
    /// array or table identifier is not indexed, it refers to the location of its zero’th
    /// element (which may be conceptual).
    SECTION("4-8-1")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-4/4-8-1.cor", "COMPLIANCE_DATA_DIR");
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
        CHECK_FALSE(errors);
        ss.flush();
        std::cout << reporterString << "\n";

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("foureightone", context);
        REQUIRE(module.get() != nullptr); // Needs to be REQUIRE

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        CHECK_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr); // Needs to be REQUIRE

        // Run the `main` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr); // Needs to be REQUIRE

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // base
        auto variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("baseOverlay"));
        REQUIRE(variableAddress != nullptr);
        // Dereference to get the actual value
        int32_t value = *static_cast<int32_t*>(variableAddress);
        REQUIRE(value == 10);

        // base1
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("base1Overlay"));
        REQUIRE(variableAddress != nullptr);
        // Dereference to get the actual value
        value = *static_cast<int32_t*>(variableAddress);
        REQUIRE(value == 20);

        // fixedBase
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("fixedBaseOverlay"));
        REQUIRE(variableAddress != nullptr);
        // Dereference to get the actual value
        value = *static_cast<int32_t*>(variableAddress);
        REQUIRE(value == 280);

        // fixedBaseInteger
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("fixedBaseIntegerOverlay"));
        REQUIRE(variableAddress != nullptr);
        // Dereference to get the actual value
        value = *static_cast<int32_t*>(variableAddress);
        REQUIRE(value == 280);

        // Floating
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("floatBaseOverlay"));
        REQUIRE(variableAddress != nullptr);
        // Dereference to get the actual value
        value = *static_cast<int32_t*>(variableAddress);
        REQUIRE(value == 1073741824);

        // Arrays

        // Array overlay with array
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("arrayBaseOverlay"));
        REQUIRE(variableAddress != nullptr);

        auto elements = static_cast<int32_t*>(variableAddress);

        // Check the elements from the array overlay
        // arrayBase[1] corresponds to memory offset 0 because lower bound is 1 (index - lower_bound = 0)
        CHECK(elements[0] == 10);
        CHECK(elements[2] == 30);

        // array base integer overlay
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("arrayBaseIntegerOverlay"));
        REQUIRE(variableAddress != nullptr);
        // Dereference to get the actual value
        value = *static_cast<int32_t*>(variableAddress);
        REQUIRE(value == 20);

        // Tables

        // Table overlay with table
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("baseTable"));
        REQUIRE(variableAddress != nullptr);

        std::cout << "arrayBase=" << ee->getGlobalValueAddress("arrayBase")
                  << " baseTable=" << ee->getGlobalValueAddress("baseTable")
                  << " baseTableOverlay=" << ee->getGlobalValueAddress("baseTableOverlay") << std::endl;

        elements = static_cast<int32_t*>(variableAddress);

        // Check the elements from the table overlay
        CHECK(elements[0] == 40);
        CHECK(elements[1] == 50);

        // bt1Overlay
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("bti1Overlay"));
        REQUIRE(variableAddress != nullptr);
        // Dereference to get the actual value
        value = *static_cast<int32_t*>(variableAddress);
        REQUIRE(value == 50);

        // bt1ArrayOverlay
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("bt1ArrayOverlay"));
        REQUIRE(variableAddress != nullptr);

        elements = static_cast<int32_t*>(variableAddress);

        // Check the elements from the array overlay
        // bt1ArrayOverlay[1] corresponds to memory offset 0 from the alias.
        // Since bt1ArrayOverlay is 1-based and baseTable is 0-based, their 0'th elements align.
        // Therefore bt1ArrayOverlay[1] aligns with baseTable[1] (which is 50).
        CHECK(elements[0] == 50);
        CHECK(elements[1] == 0);

        // Clean up
        delete ee;
    }

    /// An overlay base is a data reference which has previously been covered by a
    /// declaration in the same COMMON communicator.
    SECTION("4-8-2")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-4/4-8-2.cor", "COMPLIANCE_DATA_DIR");
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
        CHECK_FALSE(errors);
        ss.flush();
        std::cout << reporterString << "\n";

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("foureighttwo", context);
        REQUIRE(module.get() != nullptr); // Needs to be REQUIRE

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        CHECK_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr); // Needs to be REQUIRE

        // Run the `main` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr); // Needs to be REQUIRE

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // baseOverlay
        // Note that as this is declared in a COMMON communicator the variable name is now mangled.
        auto variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("_c66_common_baseOverlay"));
        REQUIRE(variableAddress != nullptr);
        // Dereference to get the actual value
        int32_t value = *static_cast<int32_t*>(variableAddress);
        REQUIRE(value == 10);

        // array base integer overlay
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("_c66_common_arrayBaseIntegerOverlay"));
        REQUIRE(variableAddress != nullptr);
        // Dereference to get the actual value
        value = *static_cast<int32_t*>(variableAddress);
        REQUIRE(value == 2);

        // Clean up
        delete ee;
    }
}
