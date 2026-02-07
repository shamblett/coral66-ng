/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "Syntax.hpp"

namespace coral66::Syntax
{

Node::Node(Lexer::CTokenIterator begin, Lexer::CTokenIterator end) : m_begin(begin), m_end(end) {}

Lexer::CTokenIterator Node::begin() const { return m_begin; }
Lexer::CTokenIterator Node::end() const { return m_end; }

// Program
Program::Program(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end, std::unique_ptr<Block>&& block)
    : Node(begin, end), m_block(std::move(block)) {}

const Block& Program::getBlock() const { return *m_block; }

// Block
Block::Block(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end,
             std::vector<std::unique_ptr<Declaration>>&& declarations,
             std::vector<std::unique_ptr<Statement>>&& statements)
    : Node(begin, end), m_declarations(std::move(declarations)), m_statements(std::move(statements)) {}

const std::vector<std::unique_ptr<Declaration>>& Block::getDeclarations() const { return m_declarations; }
const std::vector<std::unique_ptr<Statement>>& Block::getStatements() const { return m_statements; }

// VariableDeclaration
VariableDeclaration::VariableDeclaration(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end,
                                         const Lexer::CTokenIterator type, std::vector<Lexer::CTokenIterator>&& identifiers)
    : Declaration(begin, end), m_type(type), m_identifiers(std::move(identifiers)) {}

Lexer::CTokenIterator VariableDeclaration::getType() const { return m_type; }
const std::vector<Lexer::CTokenIterator>& VariableDeclaration::getIdentifiers() const { return m_identifiers; }

// ProcedureDeclaration
ProcedureDeclaration::ProcedureDeclaration(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end,
                                           const Lexer::CTokenIterator name,
                                           std::vector<std::unique_ptr<Declaration>>&& parameters,
                                           std::unique_ptr<Block>&& body)
    : Declaration(begin, end), m_name(name), m_parameters(std::move(parameters)), m_body(std::move(body)) {}

Lexer::CTokenIterator ProcedureDeclaration::getName() const { return m_name; }
const std::vector<std::unique_ptr<Declaration>>& ProcedureDeclaration::getParameters() const { return m_parameters; }
const Block& ProcedureDeclaration::getBody() const { return *m_body; }

// ArrayDeclaration
ArrayDeclaration::ArrayDeclaration(
    const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end, const Lexer::CTokenIterator type,
    const Lexer::CTokenIterator identifier,
                                   std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>&& dimensions)
    : Declaration(begin, end), m_type(type), m_identifier(identifier), m_dimensions(std::move(dimensions)) {}

Lexer::CTokenIterator ArrayDeclaration::getType() const { return m_type; }
Lexer::CTokenIterator ArrayDeclaration::getIdentifier() const { return m_identifier; }
const std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>& ArrayDeclaration::getDimensions() const { return m_dimensions; }

// TableDeclaration
TableDeclaration::TableDeclaration(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end,
                                   const Lexer::CTokenIterator identifier,
                                   std::unique_ptr<Expression>&& width, std::unique_ptr<Expression>&& length,
                                   std::vector<std::unique_ptr<Declaration>>&& elements)
    : Declaration(begin, end), m_identifier(identifier), m_width(std::move(width)), m_length(std::move(length)), m_elements(std::move(elements)) {}

Lexer::CTokenIterator TableDeclaration::getIdentifier() const { return m_identifier; }
const Expression* TableDeclaration::getWidth() const { return m_width.get(); }
const Expression* TableDeclaration::getLength() const { return m_length.get(); }
const std::vector<std::unique_ptr<Declaration>>& TableDeclaration::getElements() const { return m_elements; }

// OverlayDeclaration
OverlayDeclaration::OverlayDeclaration(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end,
                                       const Lexer::CTokenIterator base, const Lexer::CTokenIterator offset,
                                       std::unique_ptr<Declaration>&& overlayedDeclaration)
    : Declaration(begin, end), m_base(base), m_offset(offset), m_overlayedDeclaration(std::move(overlayedDeclaration)) {}

Lexer::CTokenIterator OverlayDeclaration::getBase() const { return m_base; }
Lexer::CTokenIterator OverlayDeclaration::getOffset() const { return m_offset; }
const Declaration& OverlayDeclaration::getOverlayedDeclaration() const { return *m_overlayedDeclaration; }

// AssignmentStatement
AssignmentStatement::AssignmentStatement(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end,
                                         const Lexer::CTokenIterator identifier, std::unique_ptr<Expression>&& expression)
    : Statement(begin, end), m_identifier(identifier), m_expression(std::move(expression)) {}

Lexer::CTokenIterator AssignmentStatement::getIdentifier() const { return m_identifier; }
const Expression& AssignmentStatement::getExpression() const { return *m_expression; }

// IfStatement
IfStatement::IfStatement(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end,
                         std::unique_ptr<Expression>&& condition,
                         std::unique_ptr<Statement>&& thenBranch,
                         std::unique_ptr<Statement>&& elseBranch)
    : Statement(begin, end), m_condition(std::move(condition)), m_thenBranch(std::move(thenBranch)), m_elseBranch(std::move(elseBranch)) {}

const Expression& IfStatement::getCondition() const { return *m_condition; }
const Statement& IfStatement::getThenBranch() const { return *m_thenBranch; }
const Statement* IfStatement::getElseBranch() const { return m_elseBranch.get(); }

// GotoStatement
GotoStatement::GotoStatement(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end,
                             const Lexer::CTokenIterator label)
    : Statement(begin, end), m_label(label) {}

Lexer::CTokenIterator GotoStatement::getLabel() const { return m_label; }

// ForStatement
ForStatement::ForStatement(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end,
                           Lexer::CTokenIterator variable, std::unique_ptr<Expression>&& start,
                           std::unique_ptr<Expression>&& step, std::unique_ptr<Expression>&& until,
                           std::unique_ptr<Statement>&& body)
    : Statement(begin, end), m_variable(variable), m_start(std::move(start)), m_step(std::move(step)), m_until(std::move(until)), m_body(std::move(body)) {}

Lexer::CTokenIterator ForStatement::getVariable() const { return m_variable; }
const Expression& ForStatement::getStart() const { return *m_start; }
const Expression& ForStatement::getStep() const { return *m_step; }
const Expression& ForStatement::getUntil() const { return *m_until; }
const Statement& ForStatement::getBody() const { return *m_body; }

// AnswerStatement
AnswerStatement::AnswerStatement(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end, std::unique_ptr<Expression>&& expression)
    : Statement(begin, end), m_expression(std::move(expression)) {}

const Expression& AnswerStatement::getExpression() const { return *m_expression; }

// ProcedureCallStatement
ProcedureCallStatement::ProcedureCallStatement(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end, std::unique_ptr<Expression>&& expression)
    : Statement(begin, end), m_expression(std::move(expression)) {}

const Expression& ProcedureCallStatement::getExpression() const { return *m_expression; }

// CodeStatement
CodeStatement::CodeStatement(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end) : Statement(begin, end) {}

// LiteralExpression
LiteralExpression::LiteralExpression(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end,
                                     const Lexer::CTokenIterator literal)
    : Expression(begin, end), m_literal(literal) {}

Lexer::CTokenIterator LiteralExpression::getLiteral() const { return m_literal; }

// IdentifierExpression
IdentifierExpression::IdentifierExpression(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end,
                                           const Lexer::CTokenIterator identifier)
    : Expression(begin, end), m_identifier(identifier) {}

Lexer::CTokenIterator IdentifierExpression::getIdentifier() const { return m_identifier; }

// BinaryExpression
BinaryExpression::BinaryExpression(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end,
                                   std::unique_ptr<Expression>&& left, const Lexer::CTokenIterator op, std::unique_ptr<Expression>&& right)
    : Expression(begin, end), m_left(std::move(left)), m_op(op), m_right(std::move(right)) {}

const Expression& BinaryExpression::getLeft() const { return *m_left; }
Lexer::CTokenIterator BinaryExpression::getOp() const { return m_op; }
const Expression& BinaryExpression::getRight() const { return *m_right; }

// PartWordExpression
PartWordExpression::PartWordExpression(const Lexer::CTokenIterator begin, const Lexer::CTokenIterator end,
                                       std::unique_ptr<Expression>&& base, std::unique_ptr<Expression>&& startBit,
                                       std::unique_ptr<Expression>&& width)
    : Expression(begin, end), m_base(std::move(base)), m_startBit(std::move(startBit)), m_width(std::move(width)) {}

const Expression& PartWordExpression::getBase() const { return *m_base; }
const Expression& PartWordExpression::getStartBit() const { return *m_startBit; }
const Expression& PartWordExpression::getWidth() const { return *m_width; }

} // namespace coral66::Syntax
