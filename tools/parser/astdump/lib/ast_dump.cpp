/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#include "ast_dump.hpp"

#include <rapidjson/stringbuffer.h>

namespace coral66
{
rapidjson::Value AstDump::dump(const Syntax::Program& program)
{
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("NodeType", "Program", m_alloc);

    // Dump Communicator (if it exists)
    if (const auto* comm = program.getCommunicator())
    {
        obj.AddMember("Communicator", dump(comm), m_alloc);
    }
    else
    {
        obj.AddMember("Communicator", rapidjson::Value(rapidjson::kNullType), m_alloc);
    }

    // Dump Block
    obj.AddMember("Block", dump(program.getBlock()), m_alloc);

    return obj;
}

rapidjson::Value AstDump::dump(const Syntax::Block& block)
{
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("NodeType", "Block", m_alloc);

    // Dump Declarations array
    rapidjson::Value decls(rapidjson::kArrayType);
    for (const auto& decl : block.getDeclarations())
    {
        decls.PushBack(dump(decl.get()), m_alloc);
    }
    obj.AddMember("Declarations", decls, m_alloc);

    // Dump Statements array
    rapidjson::Value stmts(rapidjson::kArrayType);
    for (const auto& stmt : block.getStatements())
    {
        stmts.PushBack(dump(stmt.get()), m_alloc);
    }
    obj.AddMember("Statements", stmts, m_alloc);

    // Dump Place References array
    rapidjson::Value places(rapidjson::kArrayType);
    for (const auto& place : block.getPlaceReferences())
    {
        places.PushBack(dump(place.get()), m_alloc);
    }
    obj.AddMember("PlaceReferences", places, m_alloc);
    return obj;
}

rapidjson::Value AstDump::dump(const Syntax::Node* node)
{
    if (!node)
        return rapidjson::Value(rapidjson::kNullType);

    rapidjson::Value obj(rapidjson::kObjectType);

    // --- Declarations ---
    if (auto* varDecl = dynamic_cast<const coral66::Syntax::VariableDeclaration*>(node))
    {
        obj.AddMember("NodeType", "VariableDeclaration", m_alloc);
        obj.AddMember("TypeToken", getTokenString(varDecl->getType()), m_alloc);
    }
    else if (auto* procDecl = dynamic_cast<const coral66::Syntax::ProcedureDeclaration*>(node))
    {
        obj.AddMember("NodeType", "ProcedureDeclaration", m_alloc);
        obj.AddMember("IsRecursive", procDecl->isRecursive(), m_alloc);

        rapidjson::Value params(rapidjson::kArrayType);
        for (const auto& param : procDecl->getParameters())
        {
            params.PushBack(dump(param.get()), m_alloc);
        }
        obj.AddMember("Parameters", params, m_alloc);

        // Recursively dump the procedure body statement/block
        obj.AddMember("Body", dump(&procDecl->getBody()), m_alloc);
    }
    // --- Statements ---
    else if (auto* assignStmt = dynamic_cast<const coral66::Syntax::AssignmentStatement*>(node))
    { // Assuming this exists based on semantic_analysis.cpp
        obj.AddMember("NodeType", "AssignmentStatement", m_alloc);
        // obj.AddMember("LHS", dump(assignStmt->getLHS()), m_alloc);
        //  obj.AddMember("RHS", dump(assignStmt->getRHS()), m_alloc);
    }
    // --- Expressions ---
    else if (auto* binExpr = dynamic_cast<const coral66::Syntax::BinaryExpression*>(node))
    { // Assuming this exists based on semantic_analysis.cpp
        obj.AddMember("NodeType", "BinaryExpression", m_alloc);
        // obj.AddMember("Left", dump(binExpr->getLeft()), m_alloc);
        //  obj.AddMember("Right", dump(binExpr->getRight()), m_alloc);
    }
    // ... Add cases for all other AST nodes ...
    else
    {
        obj.AddMember("NodeType", "UnknownNode", m_alloc);
    }

    return obj;
}

rapidjson::Value AstDump::getTokenString(const lexer::CTokenIterator& token)
{
    if (!token)
    {
        return rapidjson::Value(rapidjson::kNullType);
    }

    // 1. Get the base token name (e.g. "Identifier", "+", "Integer")
    std::string_view tokenNameView = coral66::lexer::tokenName(token->getTokenType(), false);
    std::string tokenName(tokenNameView);

    // 2. Extract the actual parsed value from the token's variant payload
    std::string tokenValueStr;
    const auto& valueVariant = token->getValue();

    std::visit(
        [&]<typename T0>(T0&& arg)
        {
            using T = std::decay_t<T0>;

            if constexpr (std::is_same_v<T, std::string>)
            {
                // Usually identifiers or string literals
                tokenValueStr = arg;
            }
            else if constexpr (std::is_same_v<T, coral66::lexer::NonCharString>)
            {
                // Specialized internal string representation
                tokenValueStr = "<NonCharString>";
            }
            else if constexpr (std::is_same_v<T, llvm::APSInt>)
            {
                // Integer literals
                llvm::SmallString<32> str;
                arg.toString(str);
                tokenValueStr = str.c_str();
            }
            else if constexpr (std::is_same_v<T, llvm::APFloat>)
            {
                // Floating point literals
                llvm::SmallString<32> str;
                arg.toString(str);
                tokenValueStr = str.c_str();
            }
            else if constexpr (std::is_same_v<T, std::monostate>)
            {
                // Token doesn't carry a payload (e.g., standard operators like '+' or ':=')
                // In this case, the token name/symbol is the value itself.
                tokenValueStr = tokenName;
            }
        },
        valueVariant);

    // 3. Format it beautifully as a JSON Object for the dump
    // Example output: { "Class": "Identifier", "Value": "myVariable" }
    //                 { "Class": "Assignment", "Value": ":=" }

    rapidjson::Value tokenObj(rapidjson::kObjectType);

    // Allocate strings using RapidJSON's allocator so they live inside the Document
    rapidjson::Value nameVal;
    nameVal.SetString(tokenName.c_str(), static_cast<rapidjson::SizeType>(tokenName.length()), m_alloc);

    rapidjson::Value valVal;
    valVal.SetString(tokenValueStr.c_str(), static_cast<rapidjson::SizeType>(tokenValueStr.length()), m_alloc);

    tokenObj.AddMember("Class", nameVal, m_alloc);
    tokenObj.AddMember("Value", valVal, m_alloc);

    return tokenObj;
}

} // namespace coral66
