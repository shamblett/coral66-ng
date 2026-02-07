/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <memory>
#include <vector>

#include "Lexer.hpp"

namespace coral66::Syntax
{

class Node
{
    Lexer::CTokenIterator m_begin;
    Lexer::CTokenIterator m_end;

public:
    Node(Lexer::CTokenIterator begin, Lexer::CTokenIterator end);
    virtual ~Node() = default;
    Node(Node&&) noexcept = default;
    Node& operator=(Node&&) = default;
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    [[nodiscard]] Lexer::CTokenIterator begin() const;
    [[nodiscard]] Lexer::CTokenIterator end() const;
};

class Block;
class Declaration;
class Statement;
class Expression;

class Program final : public Node
{
    std::unique_ptr<Block> m_block;
public:
    Program(Lexer::CTokenIterator begin, Lexer::CTokenIterator end, std::unique_ptr<Block>&& block);
    [[nodiscard]] const Block& getBlock() const;
};

class Block final : public Node
{
    std::vector<std::unique_ptr<Declaration>> m_declarations;
    std::vector<std::unique_ptr<Statement>> m_statements;
public:
    Block(Lexer::CTokenIterator begin, Lexer::CTokenIterator end,
          std::vector<std::unique_ptr<Declaration>>&& declarations,
          std::vector<std::unique_ptr<Statement>>&& statements);
    [[nodiscard]] const std::vector<std::unique_ptr<Declaration>>& getDeclarations() const;
    [[nodiscard]] const std::vector<std::unique_ptr<Statement>>& getStatements() const;
};

// Declarations
class Declaration : public Node { using Node::Node; };

class VariableDeclaration final : public Declaration
{
    Lexer::CTokenIterator m_type; // INTEGER, FLOATING, FIXED
    std::vector<Lexer::CTokenIterator> m_identifiers;
public:
    VariableDeclaration(Lexer::CTokenIterator begin, Lexer::CTokenIterator end, 
                        Lexer::CTokenIterator type, std::vector<Lexer::CTokenIterator>&& identifiers);
    [[nodiscard]] Lexer::CTokenIterator getType() const;
    [[nodiscard]] const std::vector<Lexer::CTokenIterator>& getIdentifiers() const;
};

class ProcedureDeclaration final : public Declaration
{
    Lexer::CTokenIterator m_name;
    std::vector<std::unique_ptr<Declaration>> m_parameters; 
    std::unique_ptr<Block> m_body;
public:
    ProcedureDeclaration(Lexer::CTokenIterator begin, Lexer::CTokenIterator end,
                         Lexer::CTokenIterator name,
                         std::vector<std::unique_ptr<Declaration>>&& parameters,
                         std::unique_ptr<Block>&& body);
    [[nodiscard]] Lexer::CTokenIterator getName() const;
    [[nodiscard]] const std::vector<std::unique_ptr<Declaration>>& getParameters() const;
    [[nodiscard]] const Block& getBody() const;
};

class ArrayDeclaration final : public Declaration
{
    Lexer::CTokenIterator m_type;
    Lexer::CTokenIterator m_identifier;
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> m_dimensions;
public:
    ArrayDeclaration(Lexer::CTokenIterator begin, Lexer::CTokenIterator end,
                     Lexer::CTokenIterator type, Lexer::CTokenIterator identifier,
                     std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>&& dimensions);
    [[nodiscard]] Lexer::CTokenIterator getType() const;
    [[nodiscard]] Lexer::CTokenIterator getIdentifier() const;
    [[nodiscard]] const std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>& getDimensions() const;
};

class TableDeclaration final : public Declaration
{
    Lexer::CTokenIterator m_identifier;
    std::unique_ptr<Expression> m_width;
    std::unique_ptr<Expression> m_length;
    std::vector<std::unique_ptr<Declaration>> m_elements;
public:
    TableDeclaration(Lexer::CTokenIterator begin, Lexer::CTokenIterator end, Lexer::CTokenIterator identifier,
                     std::unique_ptr<Expression>&& width, std::unique_ptr<Expression>&& length,
                     std::vector<std::unique_ptr<Declaration>>&& elements);
    [[nodiscard]] Lexer::CTokenIterator getIdentifier() const;
    [[nodiscard]] const Expression* getWidth() const;
    const Expression *getLength() const;
    [[nodiscard]] const std::vector<std::unique_ptr<Declaration>>& getElements() const;
};

class OverlayDeclaration final : public Declaration
{
    Lexer::CTokenIterator m_base;
    Lexer::CTokenIterator m_offset;
    std::unique_ptr<Declaration> m_overlayedDeclaration;
public:
    OverlayDeclaration(Lexer::CTokenIterator begin, Lexer::CTokenIterator end,
                       Lexer::CTokenIterator base, Lexer::CTokenIterator offset,
                       std::unique_ptr<Declaration>&& overlayedDeclaration);
    [[nodiscard]] Lexer::CTokenIterator getBase() const;
    [[nodiscard]] Lexer::CTokenIterator getOffset() const;
    [[nodiscard]] const Declaration& getOverlayedDeclaration() const;
};

// Statements
class Statement : public Node { using Node::Node; };

class AssignmentStatement final : public Statement
{
    Lexer::CTokenIterator m_identifier;
    std::unique_ptr<Expression> m_expression;
public:
    AssignmentStatement(Lexer::CTokenIterator begin, Lexer::CTokenIterator end, 
                        Lexer::CTokenIterator identifier, std::unique_ptr<Expression>&& expression);
    [[nodiscard]] Lexer::CTokenIterator getIdentifier() const;
    [[nodiscard]] const Expression& getExpression() const;
};

class IfStatement final : public Statement
{
    std::unique_ptr<Expression> m_condition;
    std::unique_ptr<Statement> m_thenBranch;
    std::unique_ptr<Statement> m_elseBranch; // optional
public:
    IfStatement(Lexer::CTokenIterator begin, Lexer::CTokenIterator end,
                std::unique_ptr<Expression>&& condition,
                std::unique_ptr<Statement>&& thenBranch,
                std::unique_ptr<Statement>&& elseBranch = nullptr);
    [[nodiscard]] const Expression& getCondition() const;
    [[nodiscard]] const Statement& getThenBranch() const;
    [[nodiscard]] const Statement* getElseBranch() const;
};

class GotoStatement final : public Statement
{
    Lexer::CTokenIterator m_label;
public:
    GotoStatement(Lexer::CTokenIterator begin, Lexer::CTokenIterator end, Lexer::CTokenIterator label);
    [[nodiscard]] Lexer::CTokenIterator getLabel() const;
};

class ForStatement final : public Statement
{
    Lexer::CTokenIterator m_variable;
    std::unique_ptr<Expression> m_start;
    std::unique_ptr<Expression> m_step; 
    std::unique_ptr<Expression> m_until;
    std::unique_ptr<Statement> m_body;
public:
    ForStatement(Lexer::CTokenIterator begin, Lexer::CTokenIterator end,
                 Lexer::CTokenIterator variable, std::unique_ptr<Expression>&& start,
                 std::unique_ptr<Expression>&& step, std::unique_ptr<Expression>&& until,
                 std::unique_ptr<Statement>&& body);
    [[nodiscard]] Lexer::CTokenIterator getVariable() const;
    [[nodiscard]] const Expression& getStart() const;
    [[nodiscard]] const Expression& getStep() const;
    [[nodiscard]] const Expression& getUntil() const;
    [[nodiscard]] const Statement& getBody() const;
};

class AnswerStatement final : public Statement
{
    std::unique_ptr<Expression> m_expression;
public:
    AnswerStatement(Lexer::CTokenIterator begin, Lexer::CTokenIterator end, std::unique_ptr<Expression>&& expression);
    [[nodiscard]] const Expression& getExpression() const;
};

class ProcedureCallStatement final : public Statement
{
    std::unique_ptr<Expression> m_expression;
public:
    ProcedureCallStatement(Lexer::CTokenIterator begin, Lexer::CTokenIterator end, std::unique_ptr<Expression>&& expression);
    [[nodiscard]] const Expression& getExpression() const;
};

class CodeStatement final : public Statement
{
public:
    CodeStatement(Lexer::CTokenIterator begin, Lexer::CTokenIterator end);
};

// Expressions
class Expression : public Node { using Node::Node; };

class LiteralExpression final : public Expression
{
    Lexer::CTokenIterator m_literal;
public:
    LiteralExpression(Lexer::CTokenIterator begin, Lexer::CTokenIterator end, Lexer::CTokenIterator literal);
    [[nodiscard]] Lexer::CTokenIterator getLiteral() const;
};

class IdentifierExpression final : public Expression
{
    Lexer::CTokenIterator m_identifier;
public:
    IdentifierExpression(Lexer::CTokenIterator begin, Lexer::CTokenIterator end, Lexer::CTokenIterator identifier);
    [[nodiscard]] Lexer::CTokenIterator getIdentifier() const;
};

class BinaryExpression final : public Expression
{
    std::unique_ptr<Expression> m_left;
    Lexer::CTokenIterator m_op;
    std::unique_ptr<Expression> m_right;
public:
    BinaryExpression(Lexer::CTokenIterator begin, Lexer::CTokenIterator end,
                     std::unique_ptr<Expression>&& left, Lexer::CTokenIterator op, std::unique_ptr<Expression>&& right);
    [[nodiscard]] const Expression& getLeft() const;
    [[nodiscard]] Lexer::CTokenIterator getOp() const;
    [[nodiscard]] const Expression& getRight() const;
};

class PartWordExpression final : public Expression
{
    std::unique_ptr<Expression> m_base;
    std::unique_ptr<Expression> m_startBit;
    std::unique_ptr<Expression> m_width;
public:
    PartWordExpression(Lexer::CTokenIterator begin, Lexer::CTokenIterator end,
                       std::unique_ptr<Expression>&& base, std::unique_ptr<Expression>&& startBit,
                       std::unique_ptr<Expression>&& width);
    [[nodiscard]] const Expression& getBase() const;
    [[nodiscard]] const Expression& getStartBit() const;
    [[nodiscard]] const Expression& getWidth() const;
};

}
