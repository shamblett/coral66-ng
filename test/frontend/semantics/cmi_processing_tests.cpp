/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 03/08/2026
 */

#include <deque>
#include <string>

#include "lib/frontend/semantics/cmi_processor.hpp"
#include "lib/frontend/semantics/semantics.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::semantics;
using namespace coral66::lexer;

// Utilities for synthetic tokens and types
static std::deque<CToken> g_tokens;
static std::deque<IntrusiveVariantValue<Type>> g_types;

static CTokenIterator makeToken(const std::string& name)
{
    g_tokens.emplace_back(TokenType::Identifier, 0, 0, 0, 0, name);
    return &g_tokens.back();
}

static const Type* makeType(IntrusiveVariantValue<Type> type)
{
    g_types.push_back(std::move(type));
    return g_types.back().data();
}

TEST_CASE("CMI Processor Generation", "[Semantics][CMI]")
{
    std::vector<const Usable*> exportedSymbols;

    SECTION("Version String and Header")
    {
        std::string cmiString = CMIProcessor::generateCMIString("test_lib", exportedSymbols);

        CHECK(cmiString.find("\"cmi_version\": \"1.0.0\"") != std::string::npos);
        CHECK(cmiString.find("\"library_name\": \"test_lib\"") != std::string::npos);
    }

    SECTION("Variable Generation")
    {
        auto type = IntrusiveVariantValue(PrimitiveType(PrimitiveType::Integer));
        VariableDeclaration varDecl(makeToken("my_var"), std::move(type));
        exportedSymbols.push_back(&varDecl);

        std::string cmiString = CMIProcessor::generateCMIString("test_lib", exportedSymbols);

        CHECK(cmiString.find("\"name\": \"my_var\"") != std::string::npos);
        CHECK(cmiString.find("\"symbol_category\": \"data\"") != std::string::npos);
        CHECK(cmiString.find("\"data_type\": \"INTEGER\"") != std::string::npos);
    }

    SECTION("Procedure Generation")
    {
        std::vector<ProcedureType::Parameter> params;
        params.push_back({.type = makeType(IntrusiveVariantValue(PrimitiveType(PrimitiveType::Floating))), .name = "p1", .isValue = true});

        const Type* returnType = makeType(IntrusiveVariantValue(PrimitiveType(PrimitiveType::Integer)));
        auto type = IntrusiveVariantValue(ProcedureType(returnType, std::move(params)));

        ProcedureDeclaration procDecl(makeToken("my_proc"), std::move(type));
        exportedSymbols.push_back(&procDecl);

        std::string cmiString = CMIProcessor::generateCMIString("test_lib", exportedSymbols);

        CHECK(cmiString.find("\"name\": \"my_proc\"") != std::string::npos);
        CHECK(cmiString.find("\"symbol_category\": \"procedure\"") != std::string::npos);
        CHECK(cmiString.find("\"return_type\": \"INTEGER\"") != std::string::npos);
        CHECK(cmiString.find("\"name\": \"p1\"") != std::string::npos);
        CHECK(cmiString.find("\"pass_by\": \"VALUE\"") != std::string::npos);
        CHECK(cmiString.find("\"data_type\": \"REAL\"") != std::string::npos);
    }

    SECTION("Array Generation")
    {
        const Type* elemType = makeType(IntrusiveVariantValue(PrimitiveType(PrimitiveType::Bits)));
        std::vector<std::uint64_t> dims = {10, 20};
        auto type = IntrusiveVariantValue(ArrayType(elemType, dims));

        ArrayDeclaration arrDecl(makeToken("my_arr"), std::move(type));
        exportedSymbols.push_back(&arrDecl);

        std::string cmiString = CMIProcessor::generateCMIString("test_lib", exportedSymbols);

        CHECK(cmiString.find("\"name\": \"my_arr\"") != std::string::npos);
        CHECK(cmiString.find("\"symbol_category\": \"array\"") != std::string::npos);
        CHECK(cmiString.find("\"data_type\": \"BITS\"") != std::string::npos);
        CHECK(cmiString.find("10") != std::string::npos);
        CHECK(cmiString.find("20") != std::string::npos);
    }

    SECTION("Combined Representative Example")
    {
        auto type1 = IntrusiveVariantValue(PrimitiveType(PrimitiveType::Integer));
        VariableDeclaration varDecl(makeToken("my_var"), std::move(type1));
        exportedSymbols.push_back(&varDecl);

        const Type* returnType = makeType(IntrusiveVariantValue(PrimitiveType(PrimitiveType::Floating)));
        auto type2 = IntrusiveVariantValue(ProcedureType(returnType, {}));
        ProcedureDeclaration procDecl(makeToken("my_proc"), std::move(type2));
        exportedSymbols.push_back(&procDecl);

        std::string cmiString = CMIProcessor::generateCMIString("test_lib", exportedSymbols);

        CHECK(cmiString.find("\"cmi_version\": \"1.0.0\"") != std::string::npos);
        CHECK(cmiString.find("\"name\": \"my_var\"") != std::string::npos);
        CHECK(cmiString.find("\"symbol_category\": \"data\"") != std::string::npos);
        CHECK(cmiString.find("\"name\": \"my_proc\"") != std::string::npos);
        CHECK(cmiString.find("\"symbol_category\": \"procedure\"") != std::string::npos);
    }
}

TEST_CASE("CMI Processor Parsing", "[Semantics][CMI]")
{
    std::vector<std::unique_ptr<Declaration>> importedSymbols;

    SECTION("Version Mismatch")
    {
        std::string invalidCmi = R"({
            "header": {
                "library_name": "test_lib",
                "cmi_version": "9.9.9"
            },
            "symbols": []
        })";

        bool result = CMIProcessor::parseCMIString(invalidCmi, importedSymbols);
        CHECK_FALSE(result);
        CHECK(importedSymbols.empty());
    }

    SECTION("Variable Parsing")
    {
        std::string validCmi = R"({
            "header": {
                "library_name": "test_lib",
                "cmi_version": "1.0.0"
            },
            "symbols": [
                {
                    "name": "parsed_var",
                    "symbol_category": "data",
                    "data_type": "INTEGER"
                }
            ]
        })";

        bool result = CMIProcessor::parseCMIString(validCmi, importedSymbols);
        CHECK(result);
        REQUIRE(importedSymbols.size() == 1);

        auto* decl = importedSymbols[0].get();
        CHECK(decl->getName() == "parsed_var");
        REQUIRE(decl->is<VariableDeclaration>());
        auto* varDecl = decl->tryAs<VariableDeclaration>();
        REQUIRE(varDecl->getType().is<PrimitiveType>());
        CHECK(varDecl->getType().tryAs<PrimitiveType>()->getKind() == PrimitiveType::Integer);
    }

    SECTION("Procedure Parsing")
    {
        std::string validCmi = R"({
            "header": {
                "library_name": "test_lib",
                "cmi_version": "1.0.0"
            },
            "symbols": [
                {
                    "name": "parsed_proc",
                    "symbol_category": "procedure",
                    "return_type": "REAL",
                    "parameters": [
                        {
                            "name": "p1",
                            "pass_by": "VALUE",
                            "data_type": "INTEGER"
                        }
                    ]
                }
            ]
        })";

        bool result = CMIProcessor::parseCMIString(validCmi, importedSymbols);
        CHECK(result);
        REQUIRE(importedSymbols.size() == 1);

        auto* decl = importedSymbols[0].get();
        CHECK(decl->getName() == "parsed_proc");
        REQUIRE(decl->is<ProcedureDeclaration>());
        auto* procDecl = decl->tryAs<ProcedureDeclaration>();
        REQUIRE(procDecl->getType().is<ProcedureType>());
        auto* procType = procDecl->getType().tryAs<ProcedureType>();

        REQUIRE(procType->getReturnType()->is<PrimitiveType>());
        CHECK(procType->getReturnType()->tryAs<PrimitiveType>()->getKind() == PrimitiveType::Floating);

        REQUIRE(procType->getParameters().size() == 1);
        CHECK(procType->getParameters()[0].name == "p1");
        CHECK(procType->getParameters()[0].isValue == true);
        REQUIRE(procType->getParameters()[0].type->is<PrimitiveType>());
        CHECK(procType->getParameters()[0].type->tryAs<PrimitiveType>()->getKind() == PrimitiveType::Integer);
    }

    SECTION("Array Parsing")
    {
        std::string validCmi = R"({
            "header": {
                "library_name": "test_lib",
                "cmi_version": "1.0.0"
            },
            "symbols": [
                {
                    "name": "parsed_arr",
                    "symbol_category": "array",
                    "data_type": "BITS",
                    "dimensions": [10, 20]
                }
            ]
        })";

        bool result = CMIProcessor::parseCMIString(validCmi, importedSymbols);
        CHECK(result);
        REQUIRE(importedSymbols.size() == 1);

        auto* decl = importedSymbols[0].get();
        CHECK(decl->getName() == "parsed_arr");
        REQUIRE(decl->is<ArrayDeclaration>());
        auto* arrDecl = decl->tryAs<ArrayDeclaration>();
        REQUIRE(arrDecl->getType().is<ArrayType>());
        auto* arrType = arrDecl->getType().tryAs<ArrayType>();

        REQUIRE(arrType->getElementType().is<PrimitiveType>());
        CHECK(arrType->getElementType().tryAs<PrimitiveType>()->getKind() == PrimitiveType::Bits);

        REQUIRE(arrType->getDimensions().size() == 2);
        CHECK(arrType->getDimensions()[0] == 10);
        CHECK(arrType->getDimensions()[1] == 20);
    }

    SECTION("Combined Representative Example")
    {
        std::string validCmi = R"({
            "header": {
                "library_name": "test_lib",
                "cmi_version": "1.0.0"
            },
            "symbols": [
                {
                    "name": "var1",
                    "symbol_category": "data",
                    "data_type": "INTEGER"
                },
                {
                    "name": "proc1",
                    "symbol_category": "procedure",
                    "return_type": "REAL",
                    "parameters": []
                }
            ]
        })";

        bool result = CMIProcessor::parseCMIString(validCmi, importedSymbols);
        CHECK(result);
        REQUIRE(importedSymbols.size() == 2);

        CHECK(importedSymbols[0]->getName() == "var1");
        CHECK(importedSymbols[1]->getName() == "proc1");
    }
}
