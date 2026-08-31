/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#include "ast_dump.hpp"

#include "lib/support/vendor/rapidjson/stringbuffer.h"

namespace coral66::tools
{

rapidjson::Value AstDump::dump(const Expression* expression)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    if (auto* literalExpression = dynamic_cast<const LiteralExpression*>(expression))
    {
        obj.AddMember("NodeType", "LiteralExpression", m_alloc);
        obj.AddMember("Literal", getTokenString(literalExpression->getLiteral()), m_alloc);
    }
    else if (auto* identifierExpression = dynamic_cast<const IdentifierExpression*>(expression))
    {
        obj.AddMember("NodeType", "IdentifierExpression", m_alloc);
        obj.AddMember("Identifier", getTokenString(identifierExpression->getIdentifier()), m_alloc);
    }
    else if (auto* binaryExpression = dynamic_cast<const BinaryExpression*>(expression))
    {
        obj.AddMember("NodeType", "BinaryExpression", m_alloc);
        obj.AddMember("Left", dump(&binaryExpression->getLeft()), m_alloc);
        obj.AddMember("Operator", getTokenString(binaryExpression->getOp()), m_alloc);
        obj.AddMember("Right", dump(&binaryExpression->getRight()), m_alloc);
    }
    else if (auto* partWordExpression = dynamic_cast<const PartWordExpression*>(expression))
    {
        obj.AddMember("NodeType", "PartWordExpression", m_alloc);
        obj.AddMember("Base", dump(&partWordExpression->getBase()), m_alloc);
        obj.AddMember("StartBit", dump(&partWordExpression->getStartBit()), m_alloc);
        obj.AddMember("Width", dump(&partWordExpression->getWidth()), m_alloc);
    }
    else if (auto* unaryExpression = dynamic_cast<const UnaryExpression*>(expression))
    {
        obj.AddMember("NodeType", "UnaryExpression", m_alloc);
        obj.AddMember("Op", getTokenString(unaryExpression->getOp()), m_alloc);
        obj.AddMember("Operand", dump(&unaryExpression->getOperand()), m_alloc);
    }
    else if (auto* conditionalExpression = dynamic_cast<const ConditionalExpression*>(expression))
    {
        obj.AddMember("NodeType", "ConditionalExpression", m_alloc);
        obj.AddMember("Condition", dump(&conditionalExpression->getCondition()), m_alloc);
        obj.AddMember("Then", dump(&conditionalExpression->getThenBranch()), m_alloc);
        obj.AddMember("Else", dump(&conditionalExpression->getElseBranch()), m_alloc);
    }
    else if (auto* functionCallExpression = dynamic_cast<const FunctionCallExpression*>(expression))
    {
        obj.AddMember("NodeType", "FunctionCallExpression", m_alloc);
        obj.AddMember("Function", getTokenString(functionCallExpression->getFunction()), m_alloc);

        rapidjson::Value arguments(rapidjson::kArrayType);
        for (const auto& argument : functionCallExpression->getArguments())
        {
            arguments.PushBack(dump(argument.get()), m_alloc);
        }
        obj.AddMember("Arguments", arguments, m_alloc);
    }
    else if (auto* arrayAccessExpression = dynamic_cast<const ArrayAccessExpression*>(expression))
    {
        obj.AddMember("NodeType", "ArrayAccessExpression", m_alloc);
        obj.AddMember("Array", getTokenString(arrayAccessExpression->getArray()), m_alloc);

        rapidjson::Value indices(rapidjson::kArrayType);
        for (const auto& index : arrayAccessExpression->getIndices())
        {
            indices.PushBack(dump(index.get()), m_alloc);
        }
        obj.AddMember("Indices", indices, m_alloc);
    }
    else if (auto* locationExpression = dynamic_cast<const LocationExpression*>(expression))
    {
        obj.AddMember("NodeType", "LocationExpression", m_alloc);
        obj.AddMember("Word Reference", dump(&locationExpression->getWordReference()), m_alloc);
    }
    else if (auto* typeChangeExpression = dynamic_cast<const TypeChangeExpression*>(expression))
    {
        obj.AddMember("NodeType", "TypeChangeExpression", m_alloc);
        obj.AddMember("Type", getTokenString(typeChangeExpression->getType()), m_alloc);
        obj.AddMember("Total Bits", dump(typeChangeExpression->getTotalBits()), m_alloc);
        obj.AddMember("Fraction Bits", dump(typeChangeExpression->getFractionBits()), m_alloc);
    }

    return obj;
}

} // namespace coral66::tools