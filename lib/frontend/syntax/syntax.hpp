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

class Block;
class Declaration;
class Statement;
class Expression;
class PlaceReference;
class Communicator;

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

class Program final : public Node
{
    std::unique_ptr<Communicator> m_communicator;
    std::unique_ptr<Block> m_block;
    lexer::CTokenIterator m_name;
    bool m_segment;

public:
    Program(lexer::CTokenIterator begin, lexer::CTokenIterator end, std::unique_ptr<Block>&& block,
            std::unique_ptr<Communicator>&& communicator, lexer::CTokenIterator name, bool isSegment);
    [[nodiscard]] const Block& getBlock() const;
    [[nodiscard]] bool hasBlock() const;
    [[nodiscard]] const Communicator* getCommunicator() const;
    [[nodiscard]] lexer::CTokenIterator getName() const;
    [[nodiscard]] bool getIsSegment() const;
};

class Block final : public Node
{
    std::vector<std::unique_ptr<Declaration>> m_declarations;
    std::vector<std::unique_ptr<Statement>> m_statements;
    std::vector<std::unique_ptr<PlaceReference>> m_placeReferences;

public:
    Block(lexer::CTokenIterator begin, lexer::CTokenIterator end,
          std::vector<std::unique_ptr<Declaration>>&& declarations,
          std::vector<std::unique_ptr<Statement>>&& statements,
          std::vector<std::unique_ptr<PlaceReference>>&& placeReferences);
    [[nodiscard]] const std::vector<std::unique_ptr<Declaration>>& getDeclarations() const;
    [[nodiscard]] const std::vector<std::unique_ptr<Statement>>& getStatements() const;
    [[nodiscard]] const std::vector<std::unique_ptr<PlaceReference>>& getPlaceReferences() const;
    std::vector<std::unique_ptr<Declaration>> takeDeclarations();
    std::vector<std::unique_ptr<Statement>> takeStatements();
};

// Declarations
class Declaration : public Node
{
    using Node::Node;
};

class ParameterDeclaration : public Declaration
{
public:
    enum class ParameterMode
    {
        Value,
        Location,
        Array,
        Label,
        Switch,
        Procedure,
        UntypedValue,
        UntypedLocation
    };

private:
    ParameterMode m_mode;
    std::optional<lexer::CTokenIterator> m_type;
    std::vector<lexer::CTokenIterator> m_identifiers;

public:
    ParameterDeclaration(lexer::CTokenIterator begin, lexer::CTokenIterator end, ParameterMode mode,
                         std::optional<lexer::CTokenIterator> type, std::vector<lexer::CTokenIterator>&& identifiers);
    [[nodiscard]] ParameterMode getMode() const;
    [[nodiscard]] std::optional<lexer::CTokenIterator> getType() const;
    [[nodiscard]] const std::vector<lexer::CTokenIterator>& getIdentifiers() const;
};

class VariableDeclaration final : public Declaration
{
    lexer::CTokenIterator m_type; // INTEGER, FLOATING, FIXED
    std::vector<lexer::CTokenIterator> m_identifiers;
    bool m_hasPreset;
    std::vector<lexer::CTokenIterator> m_presetTokens;

public:
    VariableDeclaration(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator type,
                        std::vector<lexer::CTokenIterator>&& identifiers, bool hasPreset = false,
                        std::vector<lexer::CTokenIterator>&& presetTokens = {});
    [[nodiscard]] lexer::CTokenIterator getType() const;
    [[nodiscard]] const std::vector<lexer::CTokenIterator>& getIdentifiers() const;
    [[nodiscard]] bool hasPreset() const;
    [[nodiscard]] const std::vector<lexer::CTokenIterator>& getPresetTokens() const;
    void setPreset(bool hasPreset, const std::vector<lexer::CTokenIterator>& tokens);
};

class ProcedureDeclaration final : public Declaration
{
    std::optional<lexer::CTokenIterator> m_typeToken;
    lexer::CTokenIterator m_name;
    std::vector<std::unique_ptr<Declaration>> m_parameters;
    std::unique_ptr<Statement> m_body;
    bool m_isRecursive;

public:
    ProcedureDeclaration(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator name,
                         std::vector<std::unique_ptr<Declaration>>&& parameters, std::unique_ptr<Statement>&& body,
                         std::optional<lexer::CTokenIterator> typeToken = std::nullopt, bool isRecursive = false);
    [[nodiscard]] lexer::CTokenIterator getName() const;
    [[nodiscard]] const std::vector<std::unique_ptr<Declaration>>& getParameters() const;
    [[nodiscard]] const Statement& getBody() const;
    [[nodiscard]] std::optional<lexer::CTokenIterator> getTypeToken() const
    {
        return m_typeToken;
    }
    [[nodiscard]] bool isRecursive() const
    {
        return m_isRecursive;
    }
};

class ArrayDeclaration final : public Declaration
{
    lexer::CTokenIterator m_type;
    std::vector<lexer::CTokenIterator> m_identifiers;
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> m_dimensions;
    bool m_hasPreset;
    std::vector<lexer::CTokenIterator> m_presetTokens;

public:
    ArrayDeclaration(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator type,
                     std::vector<lexer::CTokenIterator>&& identifiers,
                     std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>&& dimensions,
                     bool hasPreset = false, std::vector<lexer::CTokenIterator>&& presetTokens = {});
    [[nodiscard]] lexer::CTokenIterator getType() const;
    [[nodiscard]] const std::vector<lexer::CTokenIterator>& getIdentifiers() const;
    [[nodiscard]] const std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>&
        getDimensions() const;
    [[nodiscard]] bool hasPreset() const;
    [[nodiscard]] const std::vector<lexer::CTokenIterator>& getPresetTokens() const;
    void setPreset(bool hasPreset, const std::vector<lexer::CTokenIterator>& tokens);
};

class TableElementDeclaration final : public Declaration
{
    lexer::CTokenIterator m_identifier;
    std::optional<lexer::CTokenIterator> m_type; // integer, floating, fixed (can be nullopt if just a bitfield)
    bool m_unsigned;
    std::optional<lexer::CTokenIterator> m_wordPosition;
    std::optional<lexer::CTokenIterator> m_bitPosition;

public:
    TableElementDeclaration(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator identifier,
                            std::optional<lexer::CTokenIterator> type, bool isUnsigned,
                            std::optional<lexer::CTokenIterator> wordPosition,
                            std::optional<lexer::CTokenIterator> bitPosition);
    [[nodiscard]] lexer::CTokenIterator getIdentifier() const;
    [[nodiscard]] std::optional<lexer::CTokenIterator> getType() const;
    [[nodiscard]] bool isUnsigned() const;
    [[nodiscard]] std::optional<lexer::CTokenIterator> getWordPosition() const;
    [[nodiscard]] std::optional<lexer::CTokenIterator> getBitPosition() const;
};

class TableDeclaration final : public Declaration
{
    lexer::CTokenIterator m_identifier;
    std::unique_ptr<Expression> m_width;
    std::unique_ptr<Expression> m_length;
    std::vector<std::unique_ptr<Declaration>> m_elements;
    bool m_hasPreset;
    std::vector<lexer::CTokenIterator> m_presetTokens;

public:
    TableDeclaration(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator identifier,
                     std::unique_ptr<Expression>&& width, std::unique_ptr<Expression>&& length,
                     std::vector<std::unique_ptr<Declaration>>&& elements, bool hasPreset = false,
                     std::vector<lexer::CTokenIterator>&& presetTokens = {});
    [[nodiscard]] lexer::CTokenIterator getIdentifier() const;
    [[nodiscard]] const Expression* getWidth() const;
    [[nodiscard]] const Expression* getLength() const;
    [[nodiscard]] const std::vector<std::unique_ptr<Declaration>>& getElements() const;
    [[nodiscard]] bool hasPreset() const;
    [[nodiscard]] const std::vector<lexer::CTokenIterator>& getPresetTokens() const;
};

class OverlayDeclaration final : public Declaration
{
    lexer::CTokenIterator m_base;
    lexer::CTokenIterator m_offset;
    std::optional<lexer::CTokenIterator> m_offset2;
    std::unique_ptr<Declaration> m_overlayDeclaration;

public:
    OverlayDeclaration(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator base,
                       lexer::CTokenIterator offset, std::optional<lexer::CTokenIterator> offset2,
                       std::unique_ptr<Declaration>&& overlayDeclaration);
    [[nodiscard]] lexer::CTokenIterator getBase() const;
    [[nodiscard]] lexer::CTokenIterator getOffset() const;
    [[nodiscard]] std::optional<lexer::CTokenIterator> getOffset2() const;
    [[nodiscard]] const Declaration& getOverlayDeclaration() const;
};

class PlaceSpecification final : public Declaration
{
    std::vector<lexer::CTokenIterator> m_identifiers;

public:
    PlaceSpecification(lexer::CTokenIterator begin, lexer::CTokenIterator end,
                       std::vector<lexer::CTokenIterator>&& identifiers);

    [[nodiscard]] const std::vector<lexer::CTokenIterator>& getIdentifiers() const;
};

class ProcedureSpecification final : public Declaration
{
    std::optional<lexer::CTokenIterator> m_type;
    lexer::CTokenIterator m_name;
    std::vector<std::unique_ptr<Declaration>> m_parameters;

public:
    ProcedureSpecification(lexer::CTokenIterator begin, lexer::CTokenIterator end,
                           std::optional<lexer::CTokenIterator> type, lexer::CTokenIterator name,
                           std::vector<std::unique_ptr<Declaration>>&& parameters);

    [[nodiscard]] std::optional<lexer::CTokenIterator> getType() const;
    [[nodiscard]] lexer::CTokenIterator getName() const;
    [[nodiscard]] const std::vector<std::unique_ptr<Declaration>>& getParameters() const;
};

// Place references
class PlaceReference : public Node
{
    using Node::Node;
};

class SwitchPlaceReference final : public PlaceReference
{
    lexer::CTokenIterator m_name;
    std::vector<lexer::CTokenIterator> m_labels;

public:
    SwitchPlaceReference(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator name,
                         std::vector<lexer::CTokenIterator> labels);
    [[nodiscard]] lexer::CTokenIterator getName() const;
    [[nodiscard]] std::vector<lexer::CTokenIterator> getLabels() const;
};

// Statements
class Statement : public Node
{
    std::vector<lexer::CTokenIterator> m_labels;

public:
    using Node::Node;
    Statement(lexer::CTokenIterator begin, lexer::CTokenIterator end, std::vector<lexer::CTokenIterator> labels)
        : Node(begin, end), m_labels(std::move(labels))
    {
    }

    void setLabels(std::vector<lexer::CTokenIterator> labels)
    {
        m_labels = std::move(labels);
    }
    [[nodiscard]] const std::vector<lexer::CTokenIterator>& getLabels() const
    {
        return m_labels;
    }
};

class DummyStatement final : public Statement
{
public:
    DummyStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end);
};

class BlockStatement final : public Statement
{
    std::unique_ptr<Block> m_block;

public:
    BlockStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end, std::unique_ptr<Block>&& block);
    [[nodiscard]] const Block& getBlock() const;
    [[nodiscard]] bool hasBlock() const;
};

class CompoundStatement final : public Statement
{
    std::vector<std::unique_ptr<Statement>> m_statements;

public:
    CompoundStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end,
                      std::vector<std::unique_ptr<Statement>>&& statements);
    [[nodiscard]] const std::vector<std::unique_ptr<Statement>>& getStatements() const;
};

class AssignmentStatement final : public Statement
{
    std::unique_ptr<Expression> m_variable;
    std::unique_ptr<Expression> m_expression;

public:
    AssignmentStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end, std::unique_ptr<Expression>&& variable,
                        std::unique_ptr<Expression>&& expression);
    [[nodiscard]] const Expression& getVariable() const;
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
    std::unique_ptr<Expression> m_index;

public:
    GotoStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end, lexer::CTokenIterator label,
                  std::unique_ptr<Expression> index);
    [[nodiscard]] lexer::CTokenIterator getLabel() const;
    [[nodiscard]] const Expression* getIndex() const;
};

struct ForElement
{
    std::unique_ptr<Expression> expression;
    std::unique_ptr<Expression> step;
    std::unique_ptr<Expression> until;
    std::unique_ptr<Expression> whileCondition;
};

class ForStatement final : public Statement
{
    std::unique_ptr<Expression> m_variable;
    std::vector<ForElement> m_elements;
    std::unique_ptr<Statement> m_body;

public:
    ForStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end, std::unique_ptr<Expression>&& variable,
                 std::vector<ForElement>&& elements, std::unique_ptr<Statement>&& body);
    [[nodiscard]] const Expression& getVariable() const;
    [[nodiscard]] const std::vector<ForElement>& getElements() const;
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
    lexer::CTokenIterator m_code;

public:
    CodeStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end);
    [[nodiscard]] lexer::CTokenIterator getCode() const;
    ;
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
                         std::unique_ptr<Expression>&& totalBits, std::unique_ptr<Expression>&& fractionBits,
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

class CommonCommunicator final : public Communicator
{
    std::vector<std::unique_ptr<Declaration>> m_items;

public:
    CommonCommunicator(lexer::CTokenIterator begin, lexer::CTokenIterator end,
                       std::vector<std::unique_ptr<Declaration>>&& items);
    [[nodiscard]] const std::vector<std::unique_ptr<Declaration>>& getItems() const;
};

class LibraryCommunicator final : public Communicator
{
    std::vector<lexer::CTokenIterator> m_items;

public:
    LibraryCommunicator(lexer::CTokenIterator begin, lexer::CTokenIterator end,
                        const std::vector<lexer::CTokenIterator>& items);
    [[nodiscard]] std::vector<lexer::CTokenIterator> getItems() const;
};

class ExternalCommunicator final : public Communicator
{
    std::vector<lexer::CTokenIterator> m_items;

public:
    ExternalCommunicator(lexer::CTokenIterator begin, lexer::CTokenIterator end,
                         const std::vector<lexer::CTokenIterator>& items);
    [[nodiscard]] std::vector<lexer::CTokenIterator> getItems() const;
};

} // namespace coral66::Syntax
