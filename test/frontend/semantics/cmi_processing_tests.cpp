/*
 * Project: coral66-ng
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
        CHECK(cmiString.find("\"communicator_type\": \"None\"") != std::string::npos);

        varDecl.setCommunicatorType(CommunicatorType::Common);
        cmiString = CMIProcessor::generateCMIString("test_lib", exportedSymbols);
        CHECK(cmiString.find("\"communicator_type\": \"Common\"") != std::string::npos);
    }

    SECTION("Procedure Generation")
    {
        std::vector<ProcedureType::Parameter> params;
        params.push_back({.type = makeType(IntrusiveVariantValue(PrimitiveType(PrimitiveType::Floating))),
                          .name = "p1",
                          .isValue = true});

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

    SECTION("Table Generation")
    {
        std::vector<Field> fields;
        fields.push_back({IntrusiveVariantValue(PrimitiveType(PrimitiveType::Integer)), "field1"});
        TableDefinition tableDef(std::move(fields), 14, 1);
        TableInfo tableInfo;
        tableInfo.name = "my_table";
        tableInfo.id = 1;
        tableInfo.type = std::move(tableDef);

        auto type = IntrusiveVariantValue(TableType(tableInfo));
        TableDeclaration tableDecl(makeToken("my_table"), std::move(type));
        exportedSymbols.push_back(&tableDecl);

        std::string cmiString = CMIProcessor::generateCMIString("test_lib", exportedSymbols);

        CHECK(cmiString.find("\"name\": \"my_table\"") != std::string::npos);
        CHECK(cmiString.find("\"symbol_category\": \"table\"") != std::string::npos);
        CHECK(cmiString.find("\"table_name\": \"my_table\"") != std::string::npos);
        CHECK(cmiString.find("\"width\": 14") != std::string::npos);
        CHECK(cmiString.find("\"length\": 1") != std::string::npos);
        CHECK(cmiString.find("\"field1\"") != std::string::npos);
    }

    SECTION("Overlay Generation")
    {
        std::vector<Field> fields;
        fields.push_back({IntrusiveVariantValue(PrimitiveType(PrimitiveType::Floating)), "ofield"});
        OverlayDefinition overlayDef(std::move(fields));
        OverlayInfo overlayInfo;
        overlayInfo.name = "my_overlay";
        overlayInfo.id = 2;
        overlayInfo.type = std::move(overlayDef);

        auto type = IntrusiveVariantValue(OverlayType(overlayInfo));
        OverlayDeclaration overlayDecl(makeToken("my_overlay"), std::move(type));
        exportedSymbols.push_back(&overlayDecl);

        std::string cmiString = CMIProcessor::generateCMIString("test_lib", exportedSymbols);

        CHECK(cmiString.find("\"name\": \"my_overlay\"") != std::string::npos);
        CHECK(cmiString.find("\"symbol_category\": \"overlay\"") != std::string::npos);
        CHECK(cmiString.find("\"overlay_name\": \"my_overlay\"") != std::string::npos);
    }

    SECTION("Table Element Generation")
    {
        TableInfo tableInfo;
        tableInfo.name = "parent_table";
        tableInfo.id = 1;
        tableInfo.type = TableDefinition({}, 10, 10);
        auto tableType = IntrusiveVariantValue(TableType(tableInfo));
        TableDeclaration parentTable(makeToken("parent_table"), std::move(tableType));

        auto type = IntrusiveVariantValue(PrimitiveType(PrimitiveType::Integer));
        TableElementDeclaration elemDecl(makeToken("my_elem"), std::move(type), true, 2, 4, &parentTable);
        exportedSymbols.push_back(&elemDecl);

        std::string cmiString = CMIProcessor::generateCMIString("test_lib", exportedSymbols);

        CHECK(cmiString.find("\"name\": \"my_elem\"") != std::string::npos);
        CHECK(cmiString.find("\"symbol_category\": \"table_element\"") != std::string::npos);
        CHECK(cmiString.find("\"data_type\": \"INTEGER\"") != std::string::npos);
        CHECK(cmiString.find("\"is_unsigned\": true") != std::string::npos);
        CHECK(cmiString.find("\"word_position\": 2") != std::string::npos);
        CHECK(cmiString.find("\"bit_position\": 4") != std::string::npos);
        CHECK(cmiString.find("\"parent_table\": \"parent_table\"") != std::string::npos);
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
                    "communicator_type": "Common",
                    "dimensions": [10, 20]
                }
            ]
        })";

        bool result = CMIProcessor::parseCMIString(validCmi, importedSymbols);
        CHECK(result);
        REQUIRE(importedSymbols.size() == 1);

        auto* decl = importedSymbols[0].get();
        CHECK(decl->getName() == "parsed_arr");
        CHECK(decl->getCommunicatorType() == CommunicatorType::Common);
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

    SECTION("Table Parsing")
    {
        std::string validCmi = R"({
            "header": {
                "library_name": "test_lib",
                "cmi_version": "1.0.0"
            },
            "symbols": [
                {
                    "name": "parsed_table",
                    "symbol_category": "table",
                    "table_name": "parsed_table",
                    "table_id": 1,
                    "width": 14,
                    "length": 1,
                    "fields": [
                        {
                            "name": "field1",
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
        CHECK(decl->getName() == "parsed_table");
        REQUIRE(decl->is<TableDeclaration>());
        auto* tableDecl = decl->tryAs<TableDeclaration>();
        REQUIRE(tableDecl->getType().is<TableType>());
        auto* tableType = tableDecl->getType().tryAs<TableType>();

        const auto& [name, id, typeInfo] = tableType->getInfo();
        CHECK(name == "parsed_table");
        REQUIRE(std::holds_alternative<TableDefinition>(typeInfo));
        const auto& def = std::get<TableDefinition>(typeInfo);
        CHECK(def.getWidth() == 14);
        CHECK(def.getLength() == 1);
        REQUIRE(def.getFields().size() == 1);
        CHECK(def.getFields()[0].name == "field1");
    }

    SECTION("Overlay Parsing")
    {
        std::string validCmi = R"({
            "header": {
                "library_name": "test_lib",
                "cmi_version": "1.0.0"
            },
            "symbols": [
                {
                    "name": "parsed_overlay",
                    "symbol_category": "overlay",
                    "overlay_name": "parsed_overlay",
                    "overlay_id": 2,
                    "fields": [
                        {
                            "name": "ofield",
                            "data_type": "REAL"
                        }
                    ]
                }
            ]
        })";

        bool result = CMIProcessor::parseCMIString(validCmi, importedSymbols);
        CHECK(result);
        REQUIRE(importedSymbols.size() == 1);

        auto* decl = importedSymbols[0].get();
        CHECK(decl->getName() == "parsed_overlay");
        REQUIRE(decl->is<OverlayDeclaration>());
        auto* overlayDecl = decl->tryAs<OverlayDeclaration>();
        REQUIRE(overlayDecl->getType().is<OverlayType>());
        auto* overlayType = overlayDecl->getType().tryAs<OverlayType>();

        const auto& [name, id, typeInfo] = overlayType->getInfo();
        CHECK(name == "parsed_overlay");
        REQUIRE(std::holds_alternative<OverlayDefinition>(typeInfo));
        const auto& def = std::get<OverlayDefinition>(typeInfo);
        REQUIRE(def.getFields().size() == 1);
        CHECK(def.getFields()[0].name == "ofield");
    }

    SECTION("Table Element Parsing")
    {
        std::string validCmi = R"({
            "header": {
                "library_name": "test_lib",
                "cmi_version": "1.0.0"
            },
            "symbols": [
                {
                    "name": "parent_table",
                    "symbol_category": "table",
                    "table_name": "parent_table",
                    "table_id": 1,
                    "width": 10,
                    "length": 10,
                    "fields": []
                },
                {
                    "name": "parsed_elem",
                    "symbol_category": "table_element",
                    "data_type": "INTEGER",
                    "is_unsigned": true,
                    "word_position": 2,
                    "bit_position": 4,
                    "parent_table": "parent_table"
                }
            ]
        })";

        bool result = CMIProcessor::parseCMIString(validCmi, importedSymbols);
        CHECK(result);
        REQUIRE(importedSymbols.size() == 2);

        auto* decl = importedSymbols[1].get();
        CHECK(decl->getName() == "parsed_elem");
        REQUIRE(decl->is<TableElementDeclaration>());
        auto* elemDecl = decl->tryAs<TableElementDeclaration>();

        REQUIRE(elemDecl->getType().is<PrimitiveType>());
        CHECK(elemDecl->getType().tryAs<PrimitiveType>()->getKind() == PrimitiveType::Integer);
        CHECK(elemDecl->isUnsigned() == true);
        REQUIRE(elemDecl->getWordPosition().has_value());
        CHECK(elemDecl->getWordPosition().value() == 2);
        REQUIRE(elemDecl->getBitPosition().has_value());
        CHECK(elemDecl->getBitPosition().value() == 4);

        REQUIRE(elemDecl->getParentTable() != nullptr);
        CHECK(elemDecl->getParentTable()->getName() == "parent_table");
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
