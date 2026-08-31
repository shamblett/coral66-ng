/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#pragma once

#include "lib/frontend/syntax/syntax.hpp"
#include "lib/support/vendor/rapidjson/document.h"

namespace coral66::tools
{
using namespace syntax;

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
    rapidjson::Value dump(const Program& program);

    // Dump to a string
    static std::string dumpToString(const Program& program);

private:
    // Node Dispatcher
    rapidjson::Value dump(const Node* node);

    // Block dump
    rapidjson::Value dump(const Block& block);

    // Communicator
    rapidjson::Value dump(const Communicator* communicator);

    // Place references

    // Declarations
    rapidjson::Value dump(const Declaration* declaration);

    // Statements
    rapidjson::Value dump(const Statement* statement);

    // For element
    rapidjson::Value dump(const ForElement& element);

    // Expressions
    rapidjson::Value dump(const Expression* expression);

    // Helper function to extract text from a Lexer CTokenIterator
    rapidjson::Value getTokenString(const lexer::CTokenIterator& token);
};

} // namespace coral66::tools