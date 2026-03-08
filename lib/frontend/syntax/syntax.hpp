/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <memory>
#include <vector>

#include "lib/frontend/lexer/lexer.hpp"

namespace coral66::Syntax
{

class Node
{
    lexer::CTokenIterator m_begin;
    lexer::CTokenIterator m_end;

public:
    Node(lexer::CTokenIterator begin, lexer::CTokenIterator end);
    virtual ~Node() = default;
    Node(Node&&) noexcept = default;
    Node& operator=(Node&&) = default;
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    [[nodiscard]] lexer::CTokenIterator begin() const;
    [[nodiscard]] lexer::CTokenIterator end() const;
};

class Block;
class Declaration;
class Statement;
class Expression;

class Program final : public Node
{
    std::unique_ptr<Block> m_block;

public:
    Program(lexer::CTokenIterator begin, lexer::CTokenIterator end, std::unique_ptr<Block>&& block);
    [[nodiscard]] const Block& getBlock() const;
};

class Block final : public Node
{
    std::vector<std::unique_ptr<Declaration>> m_declarations;
    std::vector<std::unique_ptr<Statement>> m_statements;

public:
    Block(lexer::CTokenIterator begin, lexer::CTokenIterator end,
          std::vector<std::unique_ptr<Declaration>>&& declarations,
          std::vector<std::unique_ptr<Statement>>&& statements);
    [[nodiscard]] const std::vector<std::unique_ptr<Declaration>>& getDeclarations() const;
    [[nodiscard]] const std::vector<std::unique_ptr<Statement>>& getStatements() const;
};

// Declarations
class Declaration : public Node
{
    using Node::Node;
};

class VariableDeclaration final : public Declaration
{
    lexer::CTokenIterator m_type; // INTEGER, FLOATING, FIXED
    std::vector<lexer::CTokenIterator> m_identifiers;

public:
    VariableDeclaration(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator type,
                        std::vector<lexer::CTokenIterator>&& identifiers);
    [[nodiscard]] lexer::CTokenIterator getType() const;
    [[nodiscard]] const std::vector<lexer::CTokenIterator>& getIdentifiers() const;
};

class ProcedureDeclaration final : public Declaration
{
    lexer::CTokenIterator m_name;
    std::vector<std::unique_ptr<Declaration>> m_parameters;
    std::unique_ptr<Block> m_body;

public:
    ProcedureDeclaration(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator name,
                         std::vector<std::unique_ptr<Declaration>>&& parameters, std::unique_ptr<Block>&& body);
    [[nodiscard]] lexer::CTokenIterator getName() const;
    [[nodiscard]] const std::vector<std::unique_ptr<Declaration>>& getParameters() const;
    [[nodiscard]] const Block& getBody() const;
};

class ArrayDeclaration final : public Declaration
{
    lexer::CTokenIterator m_type;
    lexer::CTokenIterator m_identifier;
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> m_dimensions;

public:
    ArrayDeclaration(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator type,
                     lexer::CTokenIterator identifier,
                     std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>&& dimensions);
    [[nodiscard]] lexer::CTokenIterator getType() const;
    [[nodiscard]] lexer::CTokenIterator getIdentifier() const;
    [[nodiscard]] const std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>&
        getDimensions() const;
};

class TableDeclaration final : public Declaration
{
    lexer::CTokenIterator m_identifier;
    std::unique_ptr<Expression> m_width;
    std::unique_ptr<Expression> m_length;
    std::vector<std::unique_ptr<Declaration>> m_elements;

public:
    TableDeclaration(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator identifier,
                     std::unique_ptr<Expression>&& width, std::unique_ptr<Expression>&& length,
                     std::vector<std::unique_ptr<Declaration>>&& elements);
    [[nodiscard]] lexer::CTokenIterator getIdentifier() const;
    [[nodiscard]] const Expression* getWidth() const;
    const Expression* getLength() const;
    [[nodiscard]] const std::vector<std::unique_ptr<Declaration>>& getElements() const;
};

class OverlayDeclaration final : public Declaration
{
    lexer::CTokenIterator m_base;
    lexer::CTokenIterator m_offset;
    std::unique_ptr<Declaration> m_overlayedDeclaration;

public:
    OverlayDeclaration(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator base,
                       lexer::CTokenIterator offset, std::unique_ptr<Declaration>&& overlayedDeclaration);
    [[nodiscard]] lexer::CTokenIterator getBase() const;
    [[nodiscard]] lexer::CTokenIterator getOffset() const;
    [[nodiscard]] const Declaration& getOverlayedDeclaration() const;
};

// Statements
class Statement : public Node
{
    using Node::Node;
};

class AssignmentStatement final : public Statement
{
    lexer::CTokenIterator m_identifier;
    std::unique_ptr<Expression> m_expression;

public:
    AssignmentStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator identifier,
                        std::unique_ptr<Expression>&& expression);
    [[nodiscard]] lexer::CTokenIterator getIdentifier() const;
    [[nodiscard]] const Expression& getExpression() const;
};

class IfStatement final : public Statement
{
    std::unique_ptr<Expression> m_condition;
    std::unique_ptr<Statement> m_thenBranch;
    std::unique_ptr<Statement> m_elseBranch; // optional
public:
    IfStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end, std::unique_ptr<Expression>&& condition,
                std::unique_ptr<Statement>&& thenBranch, std::unique_ptr<Statement>&& elseBranch = nullptr);
    [[nodiscard]] const Expression& getCondition() const;
    [[nodiscard]] const Statement& getThenBranch() const;
    [[nodiscard]] const Statement* getElseBranch() const;
};

class GotoStatement final : public Statement
{
    lexer::CTokenIterator m_label;

public:
    GotoStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator label);
    [[nodiscard]] lexer::CTokenIterator getLabel() const;
};

class ForStatement final : public Statement
{
    lexer::CTokenIterator m_variable;
    std::unique_ptr<Expression> m_start;
    std::unique_ptr<Expression> m_step;
    std::unique_ptr<Expression> m_until;
    std::unique_ptr<Statement> m_body;

public:
    ForStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator variable,
                 std::unique_ptr<Expression>&& start, std::unique_ptr<Expression>&& step,
                 std::unique_ptr<Expression>&& until, std::unique_ptr<Statement>&& body);
    [[nodiscard]] lexer::CTokenIterator getVariable() const;
    [[nodiscard]] const Expression& getStart() const;
    [[nodiscard]] const Expression& getStep() const;
    [[nodiscard]] const Expression& getUntil() const;
    [[nodiscard]] const Statement& getBody() const;
};

class AnswerStatement final : public Statement
{
    std::unique_ptr<Expression> m_expression;

public:
    AnswerStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end, std::unique_ptr<Expression>&& expression);
    [[nodiscard]] const Expression& getExpression() const;
};

class ProcedureCallStatement final : public Statement
{
    std::unique_ptr<Expression> m_expression;

public:
    ProcedureCallStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end,
                           std::unique_ptr<Expression>&& expression);
    [[nodiscard]] const Expression& getExpression() const;
};

class CodeStatement final : public Statement
{
public:
    CodeStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end);
};

// Expressions
class Expression : public Node
{
    using Node::Node;
};

class LiteralExpression final : public Expression
{
    lexer::CTokenIterator m_literal;

public:
    LiteralExpression(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator literal);
    [[nodiscard]] lexer::CTokenIterator getLiteral() const;
};

class IdentifierExpression final : public Expression
{
    lexer::CTokenIterator m_identifier;

public:
    IdentifierExpression(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator identifier);
    [[nodiscard]] lexer::CTokenIterator getIdentifier() const;
};

class BinaryExpression final : public Expression
{
    std::unique_ptr<Expression> m_left;
    lexer::CTokenIterator m_op;
    std::unique_ptr<Expression> m_right;

public:
    BinaryExpression(lexer::CTokenIterator begin, lexer::CTokenIterator end, std::unique_ptr<Expression>&& left,
                     lexer::CTokenIterator op, std::unique_ptr<Expression>&& right);
    [[nodiscard]] const Expression& getLeft() const;
    [[nodiscard]] lexer::CTokenIterator getOp() const;
    [[nodiscard]] const Expression& getRight() const;
};

class PartWordExpression final : public Expression
{
    std::unique_ptr<Expression> m_base;
    std::unique_ptr<Expression> m_startBit;
    std::unique_ptr<Expression> m_width;

public:
    PartWordExpression(lexer::CTokenIterator begin, lexer::CTokenIterator end, std::unique_ptr<Expression>&& base,
                       std::unique_ptr<Expression>&& startBit, std::unique_ptr<Expression>&& width);
    [[nodiscard]] const Expression& getBase() const;
    [[nodiscard]] const Expression& getStartBit() const;
    [[nodiscard]] const Expression& getWidth() const;
};

class UnaryExpression final : public Expression
{
    lexer::CTokenIterator m_op;
    std::unique_ptr<Expression> m_operand;

public:
    UnaryExpression(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator op,
                    std::unique_ptr<Expression>&& operand);
    [[nodiscard]] lexer::CTokenIterator getOp() const;
    [[nodiscard]] const Expression& getOperand() const;
};

class ConditionalExpression final : public Expression
{
    std::unique_ptr<Expression> m_condition;
    std::unique_ptr<Expression> m_thenBranch;
    std::unique_ptr<Expression> m_elseBranch;

public:
    ConditionalExpression(lexer::CTokenIterator begin, lexer::CTokenIterator end,
                          std::unique_ptr<Expression>&& condition, std::unique_ptr<Expression>&& thenBranch,
                          std::unique_ptr<Expression>&& elseBranch);
    [[nodiscard]] const Expression& getCondition() const;
    [[nodiscard]] const Expression& getThenBranch() const;
    [[nodiscard]] const Expression& getElseBranch() const;
};

class FunctionCallExpression final : public Expression
{
    lexer::CTokenIterator m_function;
    std::vector<std::unique_ptr<Expression>> m_arguments;

public:
    FunctionCallExpression(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator function,
                           std::vector<std::unique_ptr<Expression>>&& arguments);
    [[nodiscard]] lexer::CTokenIterator getFunction() const;
    [[nodiscard]] const std::vector<std::unique_ptr<Expression>>& getArguments() const;
};

class ArrayAccessExpression final : public Expression
{
    lexer::CTokenIterator m_array; // Can be nullptr for anonymous reference
    std::vector<std::unique_ptr<Expression>> m_indices;

public:
    ArrayAccessExpression(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator array,
                          std::vector<std::unique_ptr<Expression>>&& indices);
    [[nodiscard]] lexer::CTokenIterator getArray() const;
    [[nodiscard]] const std::vector<std::unique_ptr<Expression>>& getIndices() const;
};

class LocationExpression final : public Expression
{
    std::unique_ptr<Expression> m_wordReference;

public:
    LocationExpression(lexer::CTokenIterator begin, lexer::CTokenIterator end,
                       std::unique_ptr<Expression>&& wordReference);
    [[nodiscard]] const Expression& getWordReference() const;
};

class TypeChangeExpression final : public Expression
{
    lexer::CTokenIterator m_type;
    std::unique_ptr<Expression> m_totalBits;
    std::unique_ptr<Expression> m_fractionBits;
    std::unique_ptr<Expression> m_expression;

public:
    TypeChangeExpression(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator type,
                         std::unique_ptr<Expression>&& expression);
    TypeChangeExpression(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator type,
                         std::unique_ptr<Expression>&& totalBits,
                         std::unique_ptr<Expression>&& fractionBits,
                         std::unique_ptr<Expression>&& expression);
    [[nodiscard]] lexer::CTokenIterator getType() const;
    [[nodiscard]] const Expression* getTotalBits() const;
    [[nodiscard]] const Expression* getFractionBits() const;
    [[nodiscard]] const Expression& getExpression() const;
};

// Communicators
class Communicator : public Node
{
    using Node::Node;
};
} // namespace coral66::Syntax
