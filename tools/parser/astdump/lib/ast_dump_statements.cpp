/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#include "ast_dump.hpp"

#include <rapidjson/stringbuffer.h>

namespace coral66
{

rapidjson::Value AstDump::dump(const Syntax::Statement* statement)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    if (dynamic_cast<const Syntax::DummyStatement*>(statement))
    {
        obj.AddMember("NodeType", "DummyStatement", m_alloc);
    }
    else if (auto* blockStatement = dynamic_cast<const Syntax::BlockStatement*>(statement))
    {
        obj.AddMember("NodeType", "BlockStatement", m_alloc);
        obj.AddMember("Block", dump(&blockStatement->getBlock()), m_alloc);
    }
    else if (auto* compoundStatement = dynamic_cast<const Syntax::CompoundStatement*>(statement))
    {
        obj.AddMember("NodeType", "CompoundStatement", m_alloc);

        rapidjson::Value statements(rapidjson::kArrayType);
        for (const auto& cStatement : compoundStatement->getStatements())
        {
            statements.PushBack(dump(cStatement.get()), m_alloc);
        }
        obj.AddMember("Compound", statements, m_alloc);
    }
    else if (auto* assignmentStatement = dynamic_cast<const Syntax::AssignmentStatement*>(statement))
    {
        obj.AddMember("NodeType", "AssignmentStatement", m_alloc);
        obj.AddMember("LHS", dump(&assignmentStatement->getVariable()), m_alloc);
        obj.AddMember("RHS", dump(&assignmentStatement->getExpression()), m_alloc);
    }
    else if (auto* ifStatement = dynamic_cast<const Syntax::IfStatement*>(statement))
    {
        obj.AddMember("NodeType", "IfStatement", m_alloc);
        obj.AddMember("ThenBranch", dump(&ifStatement->getThenBranch()), m_alloc);
        obj.AddMember("ElseBranch", dump(ifStatement->getElseBranch()), m_alloc);
    }
    else if (auto* gotoStatement = dynamic_cast<const Syntax::GotoStatement*>(statement))
    {
        obj.AddMember("NodeType", "GotoStatement", m_alloc);
        obj.AddMember("Label", getTokenString(gotoStatement->getLabel()), m_alloc);
        obj.AddMember("Index", dump(gotoStatement->getIndex()), m_alloc);
    }
    else if (auto* forStatement = dynamic_cast<const Syntax::ForStatement*>(statement))
    {
        obj.AddMember("NodeType", "ForStatement", m_alloc);
        obj.AddMember("Variable", dump(&forStatement->getVariable()), m_alloc);

        rapidjson::Value elements(rapidjson::kArrayType);
        for (const auto& element : forStatement->getElements())
        {
            elements.PushBack(dump(element), m_alloc);
        }
        obj.AddMember("Elements", elements, m_alloc);

        obj.AddMember("Body", dump(&forStatement->getBody()), m_alloc);
    }

    return obj;
}

} // namespace coral66
