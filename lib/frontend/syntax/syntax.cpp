/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "syntax.hpp"

#include <utility>

namespace coral66::syntax
{
Node::Node(const lexer::CTokenIterator begin, const lexer::CTokenIterator end) : m_begin(begin), m_end(end) {}

lexer::CTokenIterator Node::begin() const
{
    return m_begin;
}
lexer::CTokenIterator Node::end() const
{
    return m_end;
}

// Program
Program::Program(const lexer::CTokenIterator begin, const lexer::CTokenIterator end, std::unique_ptr<Block>&& block,
                 std::unique_ptr<Communicator>&& communicator, const lexer::CTokenIterator name, const bool isSegment)
    : Node(begin, end),
      m_communicator(std::move(communicator)),
      m_block(std::move(block)),
      m_name(name),
      m_segment(isSegment)
{
}

bool Program::hasBlock() const
{
    return m_block != nullptr;
}

const Block& Program::getBlock() const
{
    return *m_block;
}

const Communicator* Program::getCommunicator() const
{
    return m_communicator.get();
}

lexer::CTokenIterator Program::getName() const
{
    return m_name;
}

bool Program::getIsSegment() const
{
    return m_segment;
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

std::vector<std::unique_ptr<Declaration>> Block::takeDeclarations()
{
    return std::move(m_declarations);
}

std::vector<std::unique_ptr<Statement>> Block::takeStatements()
{
    return std::move(m_statements);
}

// ParameterDeclaration

ParameterDeclaration::ParameterDeclaration(lexer::CTokenIterator begin, lexer::CTokenIterator end, ParameterMode mode,
                                           std::optional<lexer::CTokenIterator> type,
                                           std::vector<lexer::CTokenIterator>&& identifiers,
                                           std::unique_ptr<Expression> fixedTotalBits,
                                           std::unique_ptr<Expression> fixedFractionBits)

    : Declaration(begin, end), m_mode(mode), m_type(type), 
      m_fixedTotalBits(std::move(fixedTotalBits)),
      m_fixedFractionBits(std::move(fixedFractionBits)),
      m_identifiers(std::move(identifiers))

{
}

ParameterDeclaration::ParameterMode ParameterDeclaration::getMode() const

{
    return m_mode;
}

std::optional<lexer::CTokenIterator> ParameterDeclaration::getType() const

{
    return m_type;
}

const std::vector<lexer::CTokenIterator>& ParameterDeclaration::getIdentifiers() const

{
    return m_identifiers;
}

// VariableDeclaration
VariableDeclaration::VariableDeclaration(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                         const lexer::CTokenIterator type,
                                         std::vector<lexer::CTokenIterator>&& identifiers, 
                                         std::unique_ptr<Expression> fixedTotalBits,
                                         std::unique_ptr<Expression> fixedFractionBits,
                                         bool hasPreset,
                                         std::vector<lexer::CTokenIterator>&& presetTokens)
    : Declaration(begin, end),
      m_type(type),
      m_fixedTotalBits(std::move(fixedTotalBits)),
      m_fixedFractionBits(std::move(fixedFractionBits)),
      m_identifiers(std::move(identifiers)),
      m_hasPreset(hasPreset),
      m_presetTokens(std::move(presetTokens))
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
bool VariableDeclaration::hasPreset() const
{
    return m_hasPreset;
}
const std::vector<lexer::CTokenIterator>& VariableDeclaration::getPresetTokens() const
{
    return m_presetTokens;
}
void VariableDeclaration::setPreset(bool hasPreset, const std::vector<lexer::CTokenIterator>& tokens)
{
    m_hasPreset = hasPreset;
    m_presetTokens = tokens;
}

// ProcedureDeclaration
ProcedureDeclaration::ProcedureDeclaration(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                           const lexer::CTokenIterator name,
                                           std::vector<std::unique_ptr<Declaration>>&& parameters,
                                           std::unique_ptr<Statement>&& body,
                                           std::optional<lexer::CTokenIterator> typeToken, bool isRecursive)
    : Declaration(begin, end),
      m_typeToken(typeToken),
      m_name(name),
      m_parameters(std::move(parameters)),
      m_body(std::move(body)),
      m_isRecursive(isRecursive)
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
const Statement& ProcedureDeclaration::getBody() const
{
    return *m_body;
}

// ArrayDeclaration
ArrayDeclaration::ArrayDeclaration(
    const lexer::CTokenIterator begin, const lexer::CTokenIterator end, const lexer::CTokenIterator type,
    std::vector<lexer::CTokenIterator>&& identifiers,
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>&& dimensions,
    std::unique_ptr<Expression> fixedTotalBits,
    std::unique_ptr<Expression> fixedFractionBits,
    bool hasPreset,
    std::vector<lexer::CTokenIterator>&& presetTokens)
    : Declaration(begin, end),
      m_type(type),
      m_fixedTotalBits(std::move(fixedTotalBits)),
      m_fixedFractionBits(std::move(fixedFractionBits)),
      m_identifiers(std::move(identifiers)),
      m_dimensions(std::move(dimensions)),
      m_hasPreset(hasPreset),
      m_presetTokens(std::move(presetTokens))
{
}

lexer::CTokenIterator ArrayDeclaration::getType() const
{
    return m_type;
}
const std::vector<lexer::CTokenIterator>& ArrayDeclaration::getIdentifiers() const
{
    return m_identifiers;
}
const std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>&
    ArrayDeclaration::getDimensions() const
{
    return m_dimensions;
}
bool ArrayDeclaration::hasPreset() const
{
    return m_hasPreset;
}
const std::vector<lexer::CTokenIterator>& ArrayDeclaration::getPresetTokens() const
{
    return m_presetTokens;
}
void ArrayDeclaration::setPreset(bool hasPreset, const std::vector<lexer::CTokenIterator>& tokens)
{
    m_hasPreset = hasPreset;
    m_presetTokens = tokens;
}

// TableElementDeclaration
TableElementDeclaration::TableElementDeclaration(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                                 const lexer::CTokenIterator identifier,
                                                 std::optional<lexer::CTokenIterator> type, bool isUnsigned,
                                                 std::optional<lexer::CTokenIterator> wordPosition,
                                                 std::optional<lexer::CTokenIterator> bitPosition,
                                                 std::unique_ptr<Expression> fixedTotalBits,
                                                 std::unique_ptr<Expression> fixedFractionBits)
    : Declaration(begin, end),
      m_identifier(identifier),
      m_type(type),
      m_fixedTotalBits(std::move(fixedTotalBits)),
      m_fixedFractionBits(std::move(fixedFractionBits)),
      m_unsigned(isUnsigned),
      m_wordPosition(wordPosition),
      m_bitPosition(bitPosition)
{
}

lexer::CTokenIterator TableElementDeclaration::getIdentifier() const
{
    return m_identifier;
}
std::optional<lexer::CTokenIterator> TableElementDeclaration::getType() const
{
    return m_type;
}
bool TableElementDeclaration::isUnsigned() const
{
    return m_unsigned;
}
std::optional<lexer::CTokenIterator> TableElementDeclaration::getWordPosition() const
{
    return m_wordPosition;
}
std::optional<lexer::CTokenIterator> TableElementDeclaration::getBitPosition() const
{
    return m_bitPosition;
}

// TableDeclaration
TableDeclaration::TableDeclaration(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                   const lexer::CTokenIterator identifier, std::unique_ptr<Expression>&& width,
                                   std::unique_ptr<Expression>&& length,
                                   std::vector<std::unique_ptr<Declaration>>&& elements, bool hasPreset,
                                   std::vector<lexer::CTokenIterator>&& presetTokens)
    : Declaration(begin, end),
      m_identifier(identifier),
      m_width(std::move(width)),
      m_length(std::move(length)),
      m_elements(std::move(elements)),
      m_hasPreset(hasPreset),
      m_presetTokens(std::move(presetTokens))
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
bool TableDeclaration::hasPreset() const
{
    return m_hasPreset;
}
const std::vector<lexer::CTokenIterator>& TableDeclaration::getPresetTokens() const
{
    return m_presetTokens;
}

// OverlayDeclaration
OverlayDeclaration::OverlayDeclaration(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                       const lexer::CTokenIterator base, const lexer::CTokenIterator offset,
                                       std::optional<lexer::CTokenIterator> offset2,
                                       std::unique_ptr<Declaration>&& overlayedDeclaration)
    : Declaration(begin, end),
      m_base(base),
      m_offset(offset),
      m_offset2(offset2),
      m_overlayDeclaration(std::move(overlayedDeclaration))
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
std::optional<lexer::CTokenIterator> OverlayDeclaration::getOffset2() const
{
    return m_offset2;
}
const Declaration& OverlayDeclaration::getOverlayDeclaration() const
{
    return *m_overlayDeclaration;
}

// Place reference

// DummyStatement
// ReSharper disable once CppParameterMayBeConst
DummyStatement::DummyStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end)

    : Statement(begin, end)
{
}

// BlockStatement
BlockStatement::BlockStatement(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                               std::unique_ptr<Block>&& block)
    : Statement(begin, end), m_block(std::move(block))
{
}

bool BlockStatement::hasBlock() const
{
    return m_block != nullptr;
}

const Block& BlockStatement::getBlock() const
{
    return *m_block;
}

// CompoundStatement
CompoundStatement::CompoundStatement(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                     std::vector<std::unique_ptr<Statement>>&& statements)
    : Statement(begin, end), m_statements(std::move(statements))
{
}

const std::vector<std::unique_ptr<Statement>>& CompoundStatement::getStatements() const
{
    return m_statements;
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
                           std::unique_ptr<Expression>&& variable, std::vector<ForElement>&& elements,
                           std::unique_ptr<Statement>&& body)
    : Statement(begin, end), m_variable(std::move(variable)), m_elements(std::move(elements)), m_body(std::move(body))
{
}

const Expression& ForStatement::getVariable() const
{
    return *m_variable;
}
const std::vector<ForElement>& ForStatement::getElements() const
{
    return m_elements;
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

// PlaceSpecification
PlaceSpecification::PlaceSpecification(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                       std::vector<lexer::CTokenIterator>&& identifiers)
    : Declaration(begin, end), m_identifiers(std::move(identifiers))
{
}

const std::vector<lexer::CTokenIterator>& PlaceSpecification::getIdentifiers() const
{
    return m_identifiers;
}

// ProcedureSpecification
ProcedureSpecification::ProcedureSpecification(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                               std::optional<lexer::CTokenIterator> type, lexer::CTokenIterator name,
                                               std::vector<std::unique_ptr<Declaration>>&& parameters)
    : Declaration(begin, end), m_type(type), m_name(name), m_parameters(std::move(parameters))
{
}

std::optional<lexer::CTokenIterator> ProcedureSpecification::getType() const
{
    return m_type;
}

lexer::CTokenIterator ProcedureSpecification::getName() const
{
    return m_name;
}

const std::vector<std::unique_ptr<Declaration>>& ProcedureSpecification::getParameters() const
{
    return m_parameters;
}

// CommonCommunicator
CommonCommunicator::CommonCommunicator(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                       std::vector<std::unique_ptr<Declaration>>&& items)

    : Communicator(begin, end), m_items(std::move(items))
{
}

const std::vector<std::unique_ptr<Declaration>>& CommonCommunicator::getItems() const
{
    return m_items;
}

// LibraryCommunicator
LibraryCommunicator::LibraryCommunicator(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                         const std::vector<lexer::CTokenIterator>& items)

    : Communicator(begin, end), m_items(items)
{
}

std::vector<lexer::CTokenIterator> LibraryCommunicator::getItems() const
{
    return m_items;
}

// ExternalCommunicator
ExternalCommunicator::ExternalCommunicator(const lexer::CTokenIterator begin, const lexer::CTokenIterator end,
                                           const std::vector<lexer::CTokenIterator>& items)

    : Communicator(begin, end), m_items(items)
{
}

std::vector<lexer::CTokenIterator> ExternalCommunicator::getItems() const
{
    return m_items;
}

} // namespace coral66::syntax
