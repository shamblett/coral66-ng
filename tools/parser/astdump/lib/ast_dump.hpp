/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#pragma once

#include <rapidjson/document.h>

#include "lib/frontend/syntax/syntax.hpp"

namespace coral66
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
    rapidjson::Value dump(const Syntax::Program& program);

private:
    // Node Dispatcher
    rapidjson::Value dump(const Syntax::Node* node);

    // Block dump
    rapidjson::Value dump(const Syntax::Block& block);

    // Communicator
    rapidjson::Value dump(const Syntax::Communicator* communicator);

    // Place references
    rapidjson::Value dump(const Syntax::PlaceReference* placeReference);

    // Declarations
    rapidjson::Value dump(const Syntax::Declaration* declaration);

    // Statements
    rapidjson::Value dump(const Syntax::Statement* statement);

    // Expressions
    rapidjson::Value dump(const Syntax::Expression* expression);

    // Procedure call statement
    rapidjson::Value dump(const Syntax::ProcedureCallStatement* callStatement);

    // Procedure declarations
    rapidjson::Value dump(const Syntax::ProcedureDeclaration* declaration);

    // Procedure Specifications
    rapidjson::Value dump(const Syntax::ProcedureSpecification* specification);

    // Helper function to extract text from a Lexer CTokenIterator
    rapidjson::Value getTokenString(const lexer::CTokenIterator& token);
};

} // namespace coral66