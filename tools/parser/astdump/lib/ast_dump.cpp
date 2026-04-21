/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#include "ast_dump.hpp"

#include <iostream>

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

namespace coral66::tools
{

// Main entry point
rapidjson::Value AstDump::dump(const Syntax::Program& program)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    // Program or segment
    if (!program.getIsSegment())
    {
        obj.AddMember("NodeType", "Program", m_alloc);
    }
    else
    {
        obj.AddMember("NodeType", "Segment", m_alloc);
    }
    obj.AddMember("Name", getTokenString(program.getName()), m_alloc);

    // Dump Communicator
    if (const auto* communicator = program.getCommunicator())
    {
        obj.AddMember("Communicator", dump(communicator), m_alloc);
    }
    else
    {
        obj.AddMember("Communicator", rapidjson::Value(rapidjson::kNullType), m_alloc);
    }

    // Dump Block
    obj.AddMember("Block", dump(program.getBlock()), m_alloc);

    return obj;
}

// String dump
std::string AstDump::dumpToString(const Syntax::Program& program)
{
    // Create the RapidJSON Document
    rapidjson::Document doc;
    doc.SetObject();

    // Instantiate our dumper, providing the document's allocator
    AstDump dump(doc.GetAllocator());

    // Dump the AST into the RapidJSON document root
    rapidjson::Value root = dump.dump(program);
    doc.Swap(root);

    rapidjson::StringBuffer buffer;

    // Use PrettyWriter instead of Writer for human-readable output
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

// Block
rapidjson::Value AstDump::dump(const Syntax::Block& block)
{
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("NodeType", "Block", m_alloc);

    // Dump Declarations array
    rapidjson::Value declarations(rapidjson::kArrayType);
    for (const auto& declaration : block.getDeclarations())
    {
        declarations.PushBack(dump(declaration.get()), m_alloc);
    }
    if (!declarations.Empty())
    {
        obj.AddMember("Declarations", declarations, m_alloc);
    }

    // Dump Statements
    rapidjson::Value statements(rapidjson::kArrayType);
    for (const auto& statement : block.getStatements())
    {
        statements.PushBack(dump(statement.get()), m_alloc);
    }
    if (!statements.Empty())
    {
        obj.AddMember("Statements", statements, m_alloc);
    }

    // Dump Place References
    rapidjson::Value placeReferences(rapidjson::kArrayType);
    for (const auto& placeReference : block.getPlaceReferences())
    {
        placeReferences.PushBack(dump(placeReference.get()), m_alloc);
    }
    if (!placeReferences.Empty())
    {
        obj.AddMember("PlaceReferences", placeReferences, m_alloc);
    }
    return obj;
}

// Dispatcher
rapidjson::Value AstDump::dump(const Syntax::Node* node)
{
    if (!node)
        return rapidjson::Value(rapidjson::kNullType);

    rapidjson::Value obj(rapidjson::kObjectType);

    // Declarations
    if (auto* declaration = dynamic_cast<const Syntax::Declaration*>(node))
    {
        obj.AddMember("Declaration", dump(declaration), m_alloc);
    }
    // Statements
    else if (auto* statement = dynamic_cast<const Syntax::Statement*>(node))
    {
        obj.AddMember("Statement", dump(statement), m_alloc);
    }
    // Expressions
    else if (auto* expression = dynamic_cast<const Syntax::Expression*>(node))
    {
        obj.AddMember("Expression", dump(expression), m_alloc);
    }
    // Place references
    else if (auto* placeReference = dynamic_cast<const Syntax::PlaceReference*>(node))
    {
        obj.AddMember("Place Reference", dump(placeReference), m_alloc);
    }
    else
    {
        obj.AddMember("NodeType", "UnknownNode", m_alloc);
        const auto token = lexer::tokenName(node->begin()->getTokenType());
        std::cout << "ParserAstDump::WARNING - Unknown Node encountered - dump cannot resolve this node type " << token
                  << "- output is incorrect";
    }

    return obj;
}

// For element
rapidjson::Value AstDump::dump(const Syntax::ForElement& element)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    obj.AddMember("NodeType", "ForElement", m_alloc);
    obj.AddMember("Expression", dump(element.expression.get()), m_alloc);
    obj.AddMember("Step", dump(element.step.get()), m_alloc);
    obj.AddMember("Until", dump(element.until.get()), m_alloc);
    obj.AddMember("WhileCondition", dump(element.whileCondition.get()), m_alloc);

    return obj;
}

rapidjson::Value AstDump::getTokenString(const lexer::CTokenIterator& token)
{
    if (!token)
    {
        return rapidjson::Value(rapidjson::kNullType);
    }

    // Get the base token name (e.g. "Identifier", "+", "Integer")
    std::string_view tokenNameView = coral66::lexer::tokenName(token->getTokenType(), false);
    std::string tokenName(tokenNameView);

    // Extract the actual parsed value from the token's variant payload
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

    // Format it as a JSON Object for the dump
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

} // namespace coral66::tools
