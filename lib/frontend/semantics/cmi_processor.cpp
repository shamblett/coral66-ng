/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 03/08/2026
 */

#include "lib/frontend/semantics/cmi_processor.hpp"

#include <deque>
#include <fstream>

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "lib/frontend/semantics/semantics.hpp"
#include "lib/support/filesystem.hpp"

namespace coral66::semantics
{

// To satisfy the requirement for CTokenIterator (const CToken*) inside Declaration,
// we store synthetic tokens in a static container so their addresses remain valid
// throughout the semantic phase.
static std::deque<lexer::CToken> g_syntheticTokens;

// To satisfy the pointer requirements of TableType and OverlayType during CMI parse
static std::deque<TableInfo> g_syntheticTableInfos;
static std::deque<OverlayInfo> g_syntheticOverlayInfos;

static lexer::CTokenIterator createSyntheticIdentifierToken(const std::string& name)
{
    g_syntheticTokens.emplace_back(lexer::TokenType::Identifier, 0, 0, 0, 0,
                                   name // Stored as variant std::string inside the CToken
    );
    return &g_syntheticTokens.back();
}

static std::string typeToString(const Type& type)
{
    if (auto* prim = type.tryAs<PrimitiveType>())
    {
        switch (prim->getKind())
        {
            case PrimitiveType::Integer: return "INTEGER";
            case PrimitiveType::Floating: return "REAL";
            case PrimitiveType::Bits: return "BITS"; // Or part word
        }
    }
    else if (type.is<UntypedType>())
    {
        return "NONE";
    }
    return "UNKNOWN";
}

static IntrusiveVariantValue<Type> stringToType(const std::string& typeStr)
{
    if (typeStr == "INTEGER")
    {
        return PrimitiveType(PrimitiveType::Integer);
    }
    if (typeStr == "REAL")
    {
        return PrimitiveType(PrimitiveType::Floating);
    }
    if (typeStr == "BITS")
    {
        return PrimitiveType(PrimitiveType::Bits);
    }
    return UntypedType();
}

bool CMIProcessor::generateCMIFile(const std::string& libraryName, const std::vector<const Usable*>& exportedSymbols,
                                   const std::filesystem::path& outputPath)
{
    const std::string jsonContent = generateCMIString(libraryName, exportedSymbols);

    std::ofstream out(outputPath);
    if (!out.is_open())
    {
        return false;
    }
    out << jsonContent;
    return true;
}

std::string CMIProcessor::generateCMIString(const std::string& libraryName,
                                            const std::vector<const Usable*>& exportedSymbols)
{
    const rapidjson::Document doc = createDocument(libraryName, exportedSymbols);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

bool CMIProcessor::parseCMIFile(const std::filesystem::path& inputPath,
                                std::vector<std::unique_ptr<Declaration>>& importedSymbols)
{
    std::ifstream in(inputPath);
    if (!in.is_open())
    {
        return false;
    }

    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    return parseCMIString(content, importedSymbols);
}

bool CMIProcessor::parseCMIString(const std::string& cmiContent,
                                  std::vector<std::unique_ptr<Declaration>>& importedSymbols)
{
    rapidjson::Document doc;
    doc.Parse(cmiContent.c_str());

    if (doc.HasParseError())
    {
        return false;
    }

    return parseDocument(doc, importedSymbols);
}

rapidjson::Document CMIProcessor::createDocument(const std::string& libraryName,
                                                 const std::vector<const Usable*>& exportedSymbols)
{
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    // 1. Header Block
    rapidjson::Value header(rapidjson::kObjectType);
    header.AddMember("library_name", rapidjson::StringRef(libraryName.c_str()), allocator);
    header.AddMember("cmi_version", rapidjson::StringRef(CURRENT_CMI_VERSION), allocator);
    doc.AddMember("header", header, allocator);

    // 2. Symbols Block
    rapidjson::Value symbols(rapidjson::kArrayType);

    for (const Usable* usable : exportedSymbols)
    {
        auto* declaration = static_cast<const Declaration*>(usable);
        if (!declaration)
            continue;

        std::string name(declaration->getName());

        rapidjson::Value symbolObj(rapidjson::kObjectType);
        symbolObj.AddMember("name", rapidjson::Value(name.c_str(), allocator), allocator);

        if (auto* variableDeclaration = declaration->tryAs<VariableDeclaration>())
        {
            symbolObj.AddMember("symbol_category", "data", allocator);

            std::string typeStr = typeToString(variableDeclaration->getType());
            symbolObj.AddMember("data_type", rapidjson::Value(typeStr.c_str(), allocator), allocator);
        }
        else if (auto* procedureDeclaration = declaration->tryAs<ProcedureDeclaration>())
        {
            symbolObj.AddMember("symbol_category", "procedure", allocator);

            if (auto* procType = procedureDeclaration->getType().tryAs<ProcedureType>())
            {
                std::string returnTypeStr = typeToString(*procType->getReturnType());
                symbolObj.AddMember("return_type", rapidjson::Value(returnTypeStr.c_str(), allocator), allocator);

                rapidjson::Value paramsArray(rapidjson::kArrayType);
                for (const auto& [type, name1, isValue] : procType->getParameters())
                {
                    rapidjson::Value paramObj(rapidjson::kObjectType);
                    paramObj.AddMember("name", rapidjson::Value(name1.data(), name1.size(), allocator), allocator);
                    paramObj.AddMember("pass_by", rapidjson::StringRef(isValue ? "VALUE" : "LOCATION"), allocator);

                    std::string paramTypeStr = typeToString(*type);
                    paramObj.AddMember("data_type", rapidjson::Value(paramTypeStr.c_str(), allocator), allocator);

                    paramsArray.PushBack(paramObj, allocator);
                }
                symbolObj.AddMember("parameters", paramsArray, allocator);
            }
        }
        else if (auto* arrayDeclaration = declaration->tryAs<ArrayDeclaration>())
        {
            symbolObj.AddMember("symbol_category", "array", allocator);

            if (auto* arrayType = arrayDeclaration->getType().tryAs<ArrayType>())
            {
                std::string elemTypeStr = typeToString(arrayType->getElementType());
                symbolObj.AddMember("data_type", rapidjson::Value(elemTypeStr.c_str(), allocator), allocator);

                rapidjson::Value dimsArray(rapidjson::kArrayType);
                for (auto dim : arrayType->getDimensions())
                {
                    dimsArray.PushBack(dim, allocator);
                }
                symbolObj.AddMember("dimensions", dimsArray, allocator);
            }
        }
        else if (auto* tableDeclaration = declaration->tryAs<TableDeclaration>())
        {
            symbolObj.AddMember("symbol_category", "table", allocator);

            if (auto* tableType = tableDeclaration->getType().tryAs<TableType>())
            {
                const auto& [name1, id, type] = tableType->getInfo();
                symbolObj.AddMember("table_name", rapidjson::Value(name1.c_str(), allocator), allocator);
                symbolObj.AddMember("table_id", id, allocator);

                if (std::holds_alternative<TableDefinition>(type))
                {
                    const auto& def = std::get<TableDefinition>(type);
                    symbolObj.AddMember("width", def.getWidth(), allocator);
                    symbolObj.AddMember("length", def.getLength(), allocator);

                    rapidjson::Value fieldsArray(rapidjson::kArrayType);
                    for (const auto& [type1, name2] : def.getFields())
                    {
                        rapidjson::Value fieldObj(rapidjson::kObjectType);
                        fieldObj.AddMember("name", rapidjson::Value(name2.c_str(), allocator), allocator);

                        std::string fieldTypeStr = typeToString(*type1);
                        fieldObj.AddMember("data_type", rapidjson::Value(fieldTypeStr.c_str(), allocator), allocator);

                        fieldsArray.PushBack(fieldObj, allocator);
                    }
                    symbolObj.AddMember("fields", fieldsArray, allocator);
                }
            }
        }
        else if (auto* overlayDeclaration = declaration->tryAs<OverlayDeclaration>())
        {
            symbolObj.AddMember("symbol_category", "overlay", allocator);

            if (auto* overlayType = overlayDeclaration->getType().tryAs<OverlayType>())
            {
                const auto& [name1, id, type] = overlayType->getInfo();
                symbolObj.AddMember("overlay_name", rapidjson::Value(name1.c_str(), allocator), allocator);
                symbolObj.AddMember("overlay_id", id, allocator);

                if (std::holds_alternative<OverlayDefinition>(type))
                {
                    const auto& def = std::get<OverlayDefinition>(type);

                    rapidjson::Value fieldsArray(rapidjson::kArrayType);
                    for (const auto& [type1, name2] : def.getFields())
                    {
                        rapidjson::Value fieldObj(rapidjson::kObjectType);
                        fieldObj.AddMember("name", rapidjson::Value(name.c_str(), allocator), allocator);

                        std::string fieldTypeStr = typeToString(*type1);
                        fieldObj.AddMember("data_type", rapidjson::Value(fieldTypeStr.c_str(), allocator), allocator);

                        fieldsArray.PushBack(fieldObj, allocator);
                    }
                    symbolObj.AddMember("fields", fieldsArray, allocator);
                }
            }
        }
        else
        {
            // E.g., SwitchDeclaration, LabelDeclaration - Ignored for CMI export as they don't map to
            // cross-module library APIs in a meaningful way under our precompiled module model.
            continue;
        }

        symbols.PushBack(symbolObj, allocator);
    }

    doc.AddMember("symbols", symbols, allocator);
    return doc;
}

bool CMIProcessor::parseDocument(const rapidjson::Document& doc,
                                 std::vector<std::unique_ptr<Declaration>>& importedSymbols)
{
    if (!doc.HasMember("header") || !doc.HasMember("symbols"))
    {
        return false;
    }

    if (const rapidjson::Value& header = doc["header"];
        !header.HasMember("cmi_version") || std::string(header["cmi_version"].GetString()) != CURRENT_CMI_VERSION)
    {
        return false; // Version mismatch
    }

    const rapidjson::Value& symbols = doc["symbols"];
    if (!symbols.IsArray())
    {
        return false;
    }

    for (rapidjson::SizeType i = 0; i < symbols.Size(); i++)
    {
        const rapidjson::Value& sym = symbols[i];
        if (!sym.HasMember("name") || !sym.HasMember("symbol_category"))
        {
            continue;
        }

        std::string name = sym["name"].GetString();
        std::string category = sym["symbol_category"].GetString();

        lexer::CTokenIterator syntheticToken = createSyntheticIdentifierToken(name);

        if (category == "data")
        {
            if (sym.HasMember("data_type"))
            {
                std::string typeStr = sym["data_type"].GetString();
                IntrusiveVariantValue<Type> type = stringToType(typeStr);

                auto variableDeclaration = std::make_unique<VariableDeclaration>(syntheticToken, std::move(type));
                importedSymbols.push_back(std::move(variableDeclaration));
            }
        }
        else if (category == "procedure")
        {
            if (sym.HasMember("return_type") && sym.HasMember("parameters"))
            {
                std::string returnTypeStr = sym["return_type"].GetString();

                Type* returnType = new UntypedType();
                if (returnTypeStr == "INTEGER")
                    returnType = new PrimitiveType(PrimitiveType::Integer);
                else if (returnTypeStr == "REAL")
                    returnType = new PrimitiveType(PrimitiveType::Floating);
                else if (returnTypeStr == "BITS")
                    returnType = new PrimitiveType(PrimitiveType::Bits);

                std::vector<ProcedureType::Parameter> parameters;
                const rapidjson::Value& params = sym["parameters"];

                for (rapidjson::SizeType j = 0; j < params.Size(); j++)
                {
                    const rapidjson::Value& p = params[j];
                    std::string pName = p["name"].GetString();
                    bool isValue = std::string(p["pass_by"].GetString()) == "VALUE";

                    std::string pTypeStr = p["data_type"].GetString();
                    Type* pType = new UntypedType();
                    if (pTypeStr == "INTEGER")
                        pType = new PrimitiveType(PrimitiveType::Integer);
                    else if (pTypeStr == "REAL")
                        pType = new PrimitiveType(PrimitiveType::Floating);
                    else if (pTypeStr == "BITS")
                        pType = new PrimitiveType(PrimitiveType::Bits);

                    lexer::CTokenIterator paramToken = createSyntheticIdentifierToken(pName);
                    parameters.push_back({pType, paramToken->getText(), isValue});
                }

                auto procedureType =
                    IntrusiveVariantValue(std::in_place_type<ProcedureType>, returnType, std::move(parameters));
                auto procedureDeclaration =
                    std::make_unique<ProcedureDeclaration>(syntheticToken, std::move(procedureType));

                importedSymbols.push_back(std::move(procedureDeclaration));
            }
        }
        else if (category == "array")
        {
            if (sym.HasMember("data_type") && sym.HasMember("dimensions"))
            {
                std::string typeStr = sym["data_type"].GetString();

                Type* elemType = new UntypedType();
                if (typeStr == "INTEGER")
                    elemType = new PrimitiveType(PrimitiveType::Integer);
                else if (typeStr == "REAL")
                    elemType = new PrimitiveType(PrimitiveType::Floating);
                else if (typeStr == "BITS")
                    elemType = new PrimitiveType(PrimitiveType::Bits);

                std::vector<std::uint64_t> dimensions;
                const rapidjson::Value& dims = sym["dimensions"];
                for (rapidjson::SizeType j = 0; j < dims.Size(); j++)
                {
                    dimensions.push_back(dims[j].GetUint64());
                }

                auto arrayType =
                    IntrusiveVariantValue<Type>(std::in_place_type<ArrayType>, elemType, std::move(dimensions));
                auto arrayDeclaration = std::make_unique<ArrayDeclaration>(syntheticToken, std::move(arrayType));

                importedSymbols.push_back(std::move(arrayDeclaration));
            }
        }
        else if (category == "table")
        {
            if (sym.HasMember("table_name") && sym.HasMember("table_id") && sym.HasMember("width")
                && sym.HasMember("length") && sym.HasMember("fields"))
            {
                std::vector<Field> fields;
                const rapidjson::Value& fieldsArr = sym["fields"];

                for (rapidjson::SizeType j = 0; j < fieldsArr.Size(); j++)
                {
                    const rapidjson::Value& f = fieldsArr[j];
                    std::string fName = f["name"].GetString();
                    std::string fTypeStr = f["data_type"].GetString();

                    IntrusiveVariantValue<Type> fType = stringToType(fTypeStr);
                    fields.push_back({std::move(fType), fName});
                }

                TableDefinition def(std::move(fields), sym["width"].GetUint64(), sym["length"].GetUint64());

                TableInfo info;
                info.name = sym["table_name"].GetString();
                info.id = sym["table_id"].GetUint64();
                info.type = std::move(def);

                g_syntheticTableInfos.push_back(std::move(info));

                auto tableType =
                    IntrusiveVariantValue<Type>(std::in_place_type<TableType>, g_syntheticTableInfos.back());
                auto tableDeclaration = std::make_unique<TableDeclaration>(syntheticToken, std::move(tableType));

                importedSymbols.push_back(std::move(tableDeclaration));
            }
        }
        else if (category == "overlay")
        {
            if (sym.HasMember("overlay_name") && sym.HasMember("overlay_id") && sym.HasMember("fields"))
            {
                std::vector<Field> fields;
                const rapidjson::Value& fieldsArr = sym["fields"];

                for (rapidjson::SizeType j = 0; j < fieldsArr.Size(); j++)
                {
                    const rapidjson::Value& f = fieldsArr[j];
                    std::string fName = f["name"].GetString();
                    std::string fTypeStr = f["data_type"].GetString();

                    IntrusiveVariantValue<Type> fType = stringToType(fTypeStr);
                    fields.push_back({std::move(fType), fName});
                }

                OverlayDefinition def(std::move(fields));

                OverlayInfo info;
                info.name = sym["overlay_name"].GetString();
                info.id = sym["overlay_id"].GetUint64();
                info.type = std::move(def);

                g_syntheticOverlayInfos.push_back(std::move(info));

                auto overlayType =
                    IntrusiveVariantValue<Type>(std::in_place_type<OverlayType>, g_syntheticOverlayInfos.back());
                auto overlayDeclaration = std::make_unique<OverlayDeclaration>(syntheticToken, std::move(overlayType));

                importedSymbols.push_back(std::move(overlayDeclaration));
            }
        }
    }

    return true;
}

} // namespace coral66::semantics
