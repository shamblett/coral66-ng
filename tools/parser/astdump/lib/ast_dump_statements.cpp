/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#include "ast_dump.hpp"

namespace coral66::tools
{

rapidjson::Value AstDump::dump(const Statement* statement)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    if (dynamic_cast<const DummyStatement*>(statement))
    {
        obj.AddMember("NodeType", "DummyStatement", m_alloc);
    }
    else if (auto* blockStatement = dynamic_cast<const BlockStatement*>(statement))
    {
        obj.AddMember("NodeType", "BlockStatement", m_alloc);
        if (blockStatement->hasBlock())
        {
            obj.AddMember("Block", dump(blockStatement->getBlock()), m_alloc);
        }
        else
        {
            obj.AddMember("Block", rapidjson::Value(rapidjson::kNullType), m_alloc);
        }
    }
    else if (auto* compoundStatement = dynamic_cast<const CompoundStatement*>(statement))
    {
        obj.AddMember("NodeType", "CompoundStatement", m_alloc);

        rapidjson::Value statements(rapidjson::kArrayType);
        for (const auto& cStatement : compoundStatement->getStatements())
        {
            statements.PushBack(dump(cStatement.get()), m_alloc);
        }
        obj.AddMember("Compound", statements, m_alloc);
    }
    else if (auto* assignmentStatement = dynamic_cast<const AssignmentStatement*>(statement))
    {
        obj.AddMember("NodeType", "AssignmentStatement", m_alloc);
        obj.AddMember("LHS", dump(&assignmentStatement->getVariable()), m_alloc);
        obj.AddMember("RHS", dump(&assignmentStatement->getExpression()), m_alloc);
    }
    else if (auto* ifStatement = dynamic_cast<const IfStatement*>(statement))
    {
        obj.AddMember("NodeType", "IfStatement", m_alloc);
        obj.AddMember("Then", dump(&ifStatement->getThenBranch()), m_alloc);
        if (const auto* elseBranch = ifStatement->getElseBranch())
        {
            obj.AddMember("Else", dump(elseBranch), m_alloc);
        }
    }
    else if (auto* gotoStatement = dynamic_cast<const GotoStatement*>(statement))
    {
        obj.AddMember("NodeType", "GotoStatement", m_alloc);
        obj.AddMember("Label", getTokenString(gotoStatement->getLabel()), m_alloc);
        obj.AddMember("Index", dump(gotoStatement->getIndex()), m_alloc);
    }
    else if (auto* forStatement = dynamic_cast<const ForStatement*>(statement))
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
    else if (auto* answerStatement = dynamic_cast<const AnswerStatement*>(statement))
    {
        obj.AddMember("NodeType", "AnswerStatement", m_alloc);
        obj.AddMember("Expression", dump(&answerStatement->getExpression()), m_alloc);
    }
    else if (auto* procedureCallStatement = dynamic_cast<const ProcedureCallStatement*>(statement))
    {
        obj.AddMember("NodeType", "ProcedureCallStatement", m_alloc);
        obj.AddMember("Expression", dump(&procedureCallStatement->getExpression()), m_alloc);
    }
    else if (auto* codeStatement = dynamic_cast<const CodeStatement*>(statement))
    {
        obj.AddMember("NodeType", "CodeStatement", m_alloc);
        obj.AddMember("Code", getTokenString(codeStatement->getCode()), m_alloc);
    }

    return obj;
}

} // namespace coral66::tools
