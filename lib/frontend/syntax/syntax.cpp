/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "syntax.hpp"

#include <utility>

namespace coral66::Syntax
{
Node::Node(lexer::CTokenIterator begin, lexer::CTokenIterator end) : m_begin(begin), m_end(end) {}

lexer::CTokenIterator Node::begin() const
{
    return m_begin;
}
lexer::CTokenIterator Node::end() const
{
    return m_end;
}

// Program
Program::Program(const lexer::CTokenIterator begin, const lexer::CTokenIterator end, std::unique_ptr<Block>&& block)
    : Node(begin, end), m_block(std::move(block))
{
}

const Block& Program::getBlock() const
{
    return *m_block;
}

// Block
Block::Block(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
             std::vector<std::unique_ptr<Declaration>>&& declarations,
             std::vector<std::unique_ptr<Statement>>&& statements)
    : Node(begin, end), m_declarations(std::move(declarations)), m_statements(std::move(statements))
{
}

const std::vector<std::unique_ptr<Declaration>>& Block::getDeclarations() const
{
    return m_declarations;
}
const std::vector<std::unique_ptr<Statement>>& Block::getStatements() const
{
    return m_statements;
}

// VariableDeclaration
VariableDeclaration::VariableDeclaration(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                         const lexer::CTokenIterator type,
                                         std::vector<lexer::CTokenIterator>&& identifiers)
    : Declaration(begin, end), m_type(type), m_identifiers(std::move(identifiers))
{
}

lexer::CTokenIterator VariableDeclaration::getType() const
{
    return m_type;
}
const std::vector<lexer::CTokenIterator>& VariableDeclaration::getIdentifiers() const
{
    return m_identifiers;
}

// ProcedureDeclaration
ProcedureDeclaration::ProcedureDeclaration(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                           const lexer::CTokenIterator name,
                                           std::vector<std::unique_ptr<Declaration>>&& parameters,
                                           std::unique_ptr<Block>&& body)
    : Declaration(begin, end), m_name(name), m_parameters(std::move(parameters)), m_body(std::move(body))
{
}

lexer::CTokenIterator ProcedureDeclaration::getName() const
{
    return m_name;
}
const std::vector<std::unique_ptr<Declaration>>& ProcedureDeclaration::getParameters() const
{
    return m_parameters;
}
const Block& ProcedureDeclaration::getBody() const
{
    return *m_body;
}

// ArrayDeclaration
ArrayDeclaration::ArrayDeclaration(
    const lexer::CTokenIterator begin, const lexer::CTokenIterator end, const lexer::CTokenIterator type,
    const lexer::CTokenIterator identifier,
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>&& dimensions)
    : Declaration(begin, end), m_type(type), m_identifier(identifier), m_dimensions(std::move(dimensions))
{
}

lexer::CTokenIterator ArrayDeclaration::getType() const
{
    return m_type;
}
lexer::CTokenIterator ArrayDeclaration::getIdentifier() const
{
    return m_identifier;
}
const std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>&
    ArrayDeclaration::getDimensions() const
{
    return m_dimensions;
}

// TableDeclaration
TableDeclaration::TableDeclaration(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                   const lexer::CTokenIterator identifier, std::unique_ptr<Expression>&& width,
                                   std::unique_ptr<Expression>&& length,
                                   std::vector<std::unique_ptr<Declaration>>&& elements)
    : Declaration(begin, end),
      m_identifier(identifier),
      m_width(std::move(width)),
      m_length(std::move(length)),
      m_elements(std::move(elements))
{
}

lexer::CTokenIterator TableDeclaration::getIdentifier() const
{
    return m_identifier;
}
const Expression* TableDeclaration::getWidth() const
{
    return m_width.get();
}
const Expression* TableDeclaration::getLength() const
{
    return m_length.get();
}
const std::vector<std::unique_ptr<Declaration>>& TableDeclaration::getElements() const
{
    return m_elements;
}

// OverlayDeclaration
OverlayDeclaration::OverlayDeclaration(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                       const lexer::CTokenIterator base, const lexer::CTokenIterator offset,
                                       std::unique_ptr<Declaration>&& overlayedDeclaration)
    : Declaration(begin, end), m_base(base), m_offset(offset), m_overlayedDeclaration(std::move(overlayedDeclaration))
{
}

lexer::CTokenIterator OverlayDeclaration::getBase() const
{
    return m_base;
}
lexer::CTokenIterator OverlayDeclaration::getOffset() const
{
    return m_offset;
}
const Declaration& OverlayDeclaration::getOverlayedDeclaration() const
{
    return *m_overlayedDeclaration;
}

// Place reference
SwitchPlaceReference::SwitchPlaceReference(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                           const lexer::CTokenIterator name, std::vector<lexer::CTokenIterator> labels)
    : PlaceReference(begin, end), m_name(name), m_labels(std::move(labels))
{
}

lexer::CTokenIterator SwitchPlaceReference::getName() const
{
    return m_name;
}
std::vector<lexer::CTokenIterator> SwitchPlaceReference::getLabels() const
{
    return m_labels;
}

// AssignmentStatement
AssignmentStatement::AssignmentStatement(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                         std::unique_ptr<Expression>&& variable,
                                         std::unique_ptr<Expression>&& expression)
    : Statement(begin, end), m_variable(std::move(variable)), m_expression(std::move(expression))
{
}

const Expression& AssignmentStatement::getVariable() const
{
    return *m_variable;
}
const Expression& AssignmentStatement::getExpression() const
{
    return *m_expression;
}
// IfStatement
IfStatement::IfStatement(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                         std::unique_ptr<Expression>&& condition, std::unique_ptr<Statement>&& thenBranch,
                         std::unique_ptr<Statement>&& elseBranch)
    : Statement(begin, end),
      m_condition(std::move(condition)),
      m_thenBranch(std::move(thenBranch)),
      m_elseBranch(std::move(elseBranch))
{
}

const Expression& IfStatement::getCondition() const
{
    return *m_condition;
}
const Statement& IfStatement::getThenBranch() const
{
    return *m_thenBranch;
}
const Statement* IfStatement::getElseBranch() const
{
    return m_elseBranch.get();
}

// GotoStatement
GotoStatement::GotoStatement(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                             const lexer::CTokenIterator label, std::unique_ptr<Expression> index)
    : Statement(begin, end), m_label(label), m_index(std::move(index))
{
}

lexer::CTokenIterator GotoStatement::getLabel() const
{
    return m_label;
}

const Expression* GotoStatement::getIndex() const
{
    return m_index.get();
}

// ForStatement
ForStatement::ForStatement(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                           lexer::CTokenIterator variable, std::unique_ptr<Expression>&& start,
                           std::unique_ptr<Expression>&& step, std::unique_ptr<Expression>&& until,
                           std::unique_ptr<Statement>&& body)
    : Statement(begin, end),
      m_variable(variable),
      m_start(std::move(start)),
      m_step(std::move(step)),
      m_until(std::move(until)),
      m_body(std::move(body))
{
}

lexer::CTokenIterator ForStatement::getVariable() const
{
    return m_variable;
}
const Expression& ForStatement::getStart() const
{
    return *m_start;
}
const Expression& ForStatement::getStep() const
{
    return *m_step;
}
const Expression& ForStatement::getUntil() const
{
    return *m_until;
}
const Statement& ForStatement::getBody() const
{
    return *m_body;
}

// AnswerStatement
AnswerStatement::AnswerStatement(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                 std::unique_ptr<Expression>&& expression)
    : Statement(begin, end), m_expression(std::move(expression))
{
}

const Expression& AnswerStatement::getExpression() const
{
    return *m_expression;
}

// ProcedureCallStatement
ProcedureCallStatement::ProcedureCallStatement(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                               std::unique_ptr<Expression>&& expression)
    : Statement(begin, end), m_expression(std::move(expression))
{
}

const Expression& ProcedureCallStatement::getExpression() const
{
    return *m_expression;
}

// CodeStatement
CodeStatement::CodeStatement(const lexer::CTokenIterator begin, const lexer::CTokenIterator end)

    : Statement(begin, end), m_code(begin)
{
}

lexer::CTokenIterator CodeStatement::getCode() const
{
    return m_code;
}

// LiteralExpression
LiteralExpression::LiteralExpression(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                     const lexer::CTokenIterator literal)
    : Expression(begin, end), m_literal(literal)
{
}

lexer::CTokenIterator LiteralExpression::getLiteral() const
{
    return m_literal;
}

// IdentifierExpression
IdentifierExpression::IdentifierExpression(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                           const lexer::CTokenIterator identifier)
    : Expression(begin, end), m_identifier(identifier)
{
}

lexer::CTokenIterator IdentifierExpression::getIdentifier() const
{
    return m_identifier;
}

// BinaryExpression
BinaryExpression::BinaryExpression(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                   std::unique_ptr<Expression>&& left, const lexer::CTokenIterator op,
                                   std::unique_ptr<Expression>&& right)
    : Expression(begin, end), m_left(std::move(left)), m_op(op), m_right(std::move(right))
{
}

const Expression& BinaryExpression::getLeft() const
{
    return *m_left;
}
lexer::CTokenIterator BinaryExpression::getOp() const
{
    return m_op;
}
const Expression& BinaryExpression::getRight() const
{
    return *m_right;
}

// PartWordExpression
PartWordExpression::PartWordExpression(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                       std::unique_ptr<Expression>&& base, std::unique_ptr<Expression>&& startBit,
                                       std::unique_ptr<Expression>&& width)
    : Expression(begin, end), m_base(std::move(base)), m_startBit(std::move(startBit)), m_width(std::move(width))
{
}

const Expression& PartWordExpression::getBase() const
{
    return *m_base;
}
const Expression& PartWordExpression::getStartBit() const
{
    return *m_startBit;
}
const Expression& PartWordExpression::getWidth() const
{
    return *m_width;
}

// UnaryExpression
UnaryExpression::UnaryExpression(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                 const lexer::CTokenIterator op, std::unique_ptr<Expression>&& operand)
    : Expression(begin, end), m_op(op), m_operand(std::move(operand))
{
}

lexer::CTokenIterator UnaryExpression::getOp() const
{
    return m_op;
}
const Expression& UnaryExpression::getOperand() const
{
    return *m_operand;
}

// ConditionalExpression
ConditionalExpression::ConditionalExpression(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                             std::unique_ptr<Expression>&& condition,
                                             std::unique_ptr<Expression>&& thenBranch,
                                             std::unique_ptr<Expression>&& elseBranch)
    : Expression(begin, end),
      m_condition(std::move(condition)),
      m_thenBranch(std::move(thenBranch)),
      m_elseBranch(std::move(elseBranch))
{
}

const Expression& ConditionalExpression::getCondition() const
{
    return *m_condition;
}
const Expression& ConditionalExpression::getThenBranch() const
{
    return *m_thenBranch;
}
const Expression& ConditionalExpression::getElseBranch() const
{
    return *m_elseBranch;
}

// FunctionCallExpression
FunctionCallExpression::FunctionCallExpression(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                               const lexer::CTokenIterator function,
                                               std::vector<std::unique_ptr<Expression>>&& arguments)
    : Expression(begin, end), m_function(function), m_arguments(std::move(arguments))
{
}

lexer::CTokenIterator FunctionCallExpression::getFunction() const
{
    return m_function;
}
const std::vector<std::unique_ptr<Expression>>& FunctionCallExpression::getArguments() const
{
    return m_arguments;
}

// ArrayAccessExpression
ArrayAccessExpression::ArrayAccessExpression(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                             const lexer::CTokenIterator array,
                                             std::vector<std::unique_ptr<Expression>>&& indices)
    : Expression(begin, end), m_array(array), m_indices(std::move(indices))
{
}

lexer::CTokenIterator ArrayAccessExpression::getArray() const
{
    return m_array;
}
const std::vector<std::unique_ptr<Expression>>& ArrayAccessExpression::getIndices() const
{
    return m_indices;
}

// LocationExpression
LocationExpression::LocationExpression(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                       std::unique_ptr<Expression>&& wordReference)
    : Expression(begin, end), m_wordReference(std::move(wordReference))
{
}

const Expression& LocationExpression::getWordReference() const
{
    return *m_wordReference;
}

// TypeChangeExpression
TypeChangeExpression::TypeChangeExpression(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                           const lexer::CTokenIterator type, std::unique_ptr<Expression>&& expression)
    : Expression(begin, end), m_type(type), m_expression(std::move(expression))
{
}

TypeChangeExpression::TypeChangeExpression(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                           const lexer::CTokenIterator type, std::unique_ptr<Expression>&& totalBits,
                                           std::unique_ptr<Expression>&& fractionBits,
                                           std::unique_ptr<Expression>&& expression)
    : Expression(begin, end),
      m_type(type),
      m_totalBits(std::move(totalBits)),
      m_fractionBits(std::move(fractionBits)),
      m_expression(std::move(expression))
{
}

lexer::CTokenIterator TypeChangeExpression::getType() const
{
    return m_type;
}

const Expression* TypeChangeExpression::getTotalBits() const
{
    return m_totalBits.get();
}

const Expression* TypeChangeExpression::getFractionBits() const
{
    return m_fractionBits.get();
}

const Expression& TypeChangeExpression::getExpression() const
{
    return *m_expression;
}

} // namespace coral66::Syntax
