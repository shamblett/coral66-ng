/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#pragma once

#include <rapidjson/document.h>

#include "lib/frontend/syntax/syntax.hpp"

namespace coral66::tools
{

/*
 * Dumper for the parser AST.
 *
 * Outputs the dump in JSON format.
 */
class AstDump
{
    rapidjson::Document::AllocatorType& m_alloc;

public:
    explicit AstDump(rapidjson::Document::AllocatorType& allocator) : m_alloc(allocator) {}

    // Main dump method
    rapidjson::Value dump(const syntax::Program& program);

    // Dump to a string
    static std::string dumpToString(const syntax::Program& program);

private:
    // Node Dispatcher
    rapidjson::Value dump(const syntax::Node* node);

    // Block dump
    rapidjson::Value dump(const syntax::Block& block);

    // Communicator
    rapidjson::Value dump(const syntax::Communicator* communicator);

    // Place references
    rapidjson::Value dump(const syntax::PlaceReference* placeReference);

    // Declarations
    rapidjson::Value dump(const syntax::Declaration* declaration);

    // Statements
    rapidjson::Value dump(const syntax::Statement* statement);

    // For element
    rapidjson::Value dump(const syntax::ForElement& element);

    // Expressions
    rapidjson::Value dump(const syntax::Expression* expression);

    // Procedure call statement
    rapidjson::Value dump(const syntax::ProcedureCallStatement* callStatement);

    // Procedure declarations
    rapidjson::Value dump(const syntax::ProcedureDeclaration* declaration);

    // Procedure Specifications
    rapidjson::Value dump(const syntax::ProcedureSpecification* specification);

    // Helper function to extract text from a Lexer CTokenIterator
    rapidjson::Value getTokenString(const lexer::CTokenIterator& token);
};

} // namespace coral66::tools