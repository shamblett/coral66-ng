/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lib/frontend/common/semantics_common.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/lexer/lexer.hpp"
#include "lib/support/abstract_intrusive_variant.hpp"
#include "lib/support/intrusive_variant_value.hpp"
#include "lib/support/vendor/tsl/ordered_map.h"

/**
 * Defines the classes and structures for the Semantic Abstract Syntax Tree (AST) for Coral 66.
 */

namespace coral66::syntax
{
class Node;
class Program;
class Declaration;
} // namespace coral66::syntax

namespace coral66::semantics
{

class ProgramInterface;

// ============================================================================
// Forward Declarations & Enums
// ============================================================================

struct TableInfo;
struct OverlayInfo;

class TableDefinition;
class OverlayDefinition;

class ExpressionBase;
class Statement;
class Declaration;

enum class ValueCategory : std::uint8_t
{
    Lvalue,
    Rvalue
};

class ScopePoint
{
    std::size_t m_scopeId;

public:
    explicit ScopePoint(const std::size_t scopeId) : m_scopeId(scopeId) {}
    [[nodiscard]] std::size_t getScopeId() const
    {
        return m_scopeId;
    }
};

// ============================================================================
// Type Hierarchy
// ============================================================================

// ============================================================================
// Table / Overlay Definitions
// ============================================================================

struct Field
{
    IntrusiveVariantValue<Type> type;
    std::string name;
};

class TableDefinition
{
    std::vector<Field> m_fields;
    std::uint64_t m_width;
    std::uint64_t m_length;

public:
    explicit TableDefinition(std::vector<Field> fields, const std::uint64_t width, const std::uint64_t length)
        : m_fields(std::move(fields)), m_width(width), m_length(length)
    {
    }
    [[nodiscard]] const std::vector<Field>& getFields() const
    {
        return m_fields;
    }
    [[nodiscard]] std::uint64_t getWidth() const
    {
        return m_width;
    }
    [[nodiscard]] std::uint64_t getLength() const
    {
        return m_length;
    }
};

struct TableInfo
{
    std::string name;
    std::size_t id;
    std::variant<std::monostate, TableDefinition> type;
    template <class T>
    static T* getAttributeIf()
    {
        return nullptr;
    }
};

class OverlayDefinition
{
    std::vector<Field> m_fields;

public:
    explicit OverlayDefinition(std::vector<Field> fields) : m_fields(std::move(fields)) {}
};

struct OverlayInfo
{
    std::string name;
    std::size_t id;
    std::variant<std::monostate, OverlayDefinition> type;
    template <class T>
    static T* getAttributeIf()
    {
        return nullptr;
    }
};

// ============================================================================
// Expressions
// ============================================================================

class Constant;
class DeclarationRead;
class BinaryOperator;
class UnaryOperator;
class PartWordAccess;
class CallExpression;
class ErrorExpression;
class ConditionalExpression;
class ArrayAccess;
class LocationAccess;
class TypeChange;

class ExpressionBase : public AbstractIntrusiveVariant<ExpressionBase, Constant, DeclarationRead, BinaryOperator,
                                                       UnaryOperator, PartWordAccess, CallExpression, ErrorExpression,
                                                       ConditionalExpression, ArrayAccess, LocationAccess, TypeChange>
{
    IntrusiveVariantValue<Type> m_type;
    ValueCategory m_valueCategory;

protected:
    template <class T>
    ExpressionBase(std::in_place_type_t<T>, IntrusiveVariantValue<Type> type, ValueCategory const valueCategory)
        : AbstractIntrusiveVariant(std::in_place_type<T>), m_type(std::move(type)), m_valueCategory(valueCategory)
    {
    }

public:
    [[nodiscard]] const Type& getType() const
    {
        return *m_type;
    }
    [[nodiscard]] ValueCategory getValueCategory() const
    {
        return m_valueCategory;
    }
    [[nodiscard]] bool isUndefined() const
    {
        return is<ErrorExpression>();
    }
    [[nodiscard]] virtual lexer::CTokenIterator begin() const;
    [[nodiscard]] virtual lexer::CTokenIterator end() const;
    virtual ~ExpressionBase() = default;
};

class Constant final : public ExpressionBase
{
public:
    using Variant = std::variant<llvm::APSInt, llvm::APFloat, std::string>;

private:
    Variant m_value;
    lexer::CTokenIterator m_begin;
    lexer::CTokenIterator m_end;

public:
    Constant(IntrusiveVariantValue<Type> type, Variant value, const lexer::CTokenIterator begin,
             const lexer::CTokenIterator end)
        : ExpressionBase(std::in_place_type<Constant>, std::move(type), ValueCategory::Rvalue),
          m_value(std::move(value)),
          m_begin(begin),
          m_end(end)
    {
    }

    [[nodiscard]] const Variant& getValue() const
    {
        return m_value;
    }
    [[nodiscard]] lexer::CTokenIterator begin() const override
    {
        return m_begin;
    }
    [[nodiscard]] lexer::CTokenIterator end() const override
    {
        return m_end;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(Constant)
};

class Usable;

class DeclarationRead final : public ExpressionBase
{
    const Usable* CORAL66_NON_NULL m_declarationRead;
    lexer::CTokenIterator m_identifierToken;

public:
    DeclarationRead(IntrusiveVariantValue<Type> type, const Usable& usable, const lexer::CTokenIterator identifierToken)
        : ExpressionBase(std::in_place_type<DeclarationRead>, std::move(type), ValueCategory::Lvalue),
          m_declarationRead(&usable),
          m_identifierToken(identifierToken)
    {
    }

    [[nodiscard]] const Usable& getDeclarationRead() const
    {
        return *m_declarationRead;
    }
    [[nodiscard]] lexer::CTokenIterator begin() const override
    {
        return m_identifierToken;
    }
    [[nodiscard]] lexer::CTokenIterator end() const override
    {
        return m_identifierToken + 1;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(DeclarationRead)
};

class BinaryOperator final : public ExpressionBase
{
    IntrVarPtr<ExpressionBase> m_leftOperand;

public:
    enum Kind
    {
        Addition,
        Subtraction,
        Multiply,
        Divide,
        Equal,
        NotEqual,
        LessThan,
        GreaterThan,
        LessOrEqual,
        GreaterOrEqual,
        LogicAnd,
        LogicOr
    };

private:
    Kind m_kind;
    lexer::CTokenIterator m_operatorToken;
    IntrVarPtr<ExpressionBase> m_rightOperand;

public:
    BinaryOperator(IntrusiveVariantValue<Type> type, IntrVarPtr<ExpressionBase>&& left, const Kind kind,
                   const lexer::CTokenIterator opToken, IntrVarPtr<ExpressionBase>&& right)
        : ExpressionBase(std::in_place_type<BinaryOperator>, std::move(type), ValueCategory::Rvalue),
          m_leftOperand(std::move(left)),
          m_kind(kind),
          m_operatorToken(opToken),
          m_rightOperand(std::move(right))
    {
    }

    [[nodiscard]] const ExpressionBase& getLeftExpression() const
    {
        return *m_leftOperand;
    }
    [[nodiscard]] const ExpressionBase& getRightExpression() const
    {
        return *m_rightOperand;
    }
    [[nodiscard]] Kind getKind() const
    {
        return m_kind;
    }
    [[nodiscard]] lexer::CTokenIterator begin() const override;
    [[nodiscard]] lexer::CTokenIterator end() const override;
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(BinaryOperator)
};

class UnaryOperator final : public ExpressionBase
{
public:
    enum Kind
    {
        Plus,
        Minus,
        Not
    };

private:
    Kind m_kind;
    lexer::CTokenIterator m_operatorToken;
    IntrVarPtr<ExpressionBase> m_operand;

public:
    UnaryOperator(IntrusiveVariantValue<Type> type, const Kind kind, const lexer::CTokenIterator opToken,
                  IntrVarPtr<ExpressionBase>&& operand)
        : ExpressionBase(std::in_place_type<UnaryOperator>, std::move(type), ValueCategory::Rvalue),
          m_kind(kind),
          m_operatorToken(opToken),
          m_operand(std::move(operand))
    {
    }

    [[nodiscard]] Kind getKind() const
    {
        return m_kind;
    }
    [[nodiscard]] const ExpressionBase& getOperand() const
    {
        return *m_operand;
    }
    [[nodiscard]] lexer::CTokenIterator begin() const override;
    [[nodiscard]] lexer::CTokenIterator end() const override;
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(UnaryOperator)
};

class PartWordAccess final : public ExpressionBase
{
    IntrVarPtr<ExpressionBase> m_base;
    std::uint32_t m_startBit;
    std::uint32_t m_width;
    lexer::CTokenIterator m_begin;
    lexer::CTokenIterator m_end;

public:
    PartWordAccess(IntrusiveVariantValue<Type> type, IntrVarPtr<ExpressionBase>&& base, const std::uint32_t start,
                   const std::uint32_t width, const lexer::CTokenIterator beginLoc, const lexer::CTokenIterator endLoc)
        : ExpressionBase(std::in_place_type<PartWordAccess>, std::move(type), ValueCategory::Lvalue),
          m_base(std::move(base)),
          m_startBit(start),
          m_width(width),
          m_begin(beginLoc),
          m_end(endLoc)
    {
    }

    [[nodiscard]] lexer::CTokenIterator begin() const override
    {
        return m_begin;
    }
    [[nodiscard]] lexer::CTokenIterator end() const override
    {
        return m_end;
    }
    [[nodiscard]] const ExpressionBase& getBase() const
    {
        return *m_base;
    }
    [[nodiscard]] std::uint32_t getStartBit() const
    {
        return m_startBit;
    }
    [[nodiscard]] std::uint32_t getWidth() const
    {
        return m_width;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(PartWordAccess)
};

class CallExpression final : public ExpressionBase
{
    IntrVarPtr<ExpressionBase> m_functionExpression;
    std::vector<IntrVarPtr<ExpressionBase>> m_arguments;
    lexer::CTokenIterator m_begin;
    lexer::CTokenIterator m_end;

public:
    CallExpression(IntrusiveVariantValue<Type> type, IntrVarPtr<ExpressionBase>&& func,
                   std::vector<IntrVarPtr<ExpressionBase>>&& args, const lexer::CTokenIterator beginLoc,
                   const lexer::CTokenIterator endLoc)
        : ExpressionBase(std::in_place_type<CallExpression>, std::move(type), ValueCategory::Rvalue),
          m_functionExpression(std::move(func)),
          m_arguments(std::move(args)),
          m_begin(beginLoc),
          m_end(endLoc)
    {
    }

    [[nodiscard]] const ExpressionBase& getFunctionExpression() const
    {
        return *m_functionExpression;
    }
    [[nodiscard]] const std::vector<IntrVarPtr<ExpressionBase>>& getArguments() const
    {
        return m_arguments;
    }
    [[nodiscard]] lexer::CTokenIterator begin() const override
    {
        return m_begin;
    }
    [[nodiscard]] lexer::CTokenIterator end() const override
    {
        return m_end;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(CallExpression)
};

class ConditionalExpression final : public ExpressionBase
{
    IntrVarPtr<ExpressionBase> m_condition;
    IntrVarPtr<ExpressionBase> m_thenBranch;
    IntrVarPtr<ExpressionBase> m_elseBranch;

public:
    ConditionalExpression(IntrusiveVariantValue<Type> type, IntrVarPtr<ExpressionBase>&& condition,
                          IntrVarPtr<ExpressionBase>&& thenBranch, IntrVarPtr<ExpressionBase>&& elseBranch)
        : ExpressionBase(std::in_place_type<ConditionalExpression>, std::move(type), ValueCategory::Rvalue),
          m_condition(std::move(condition)),
          m_thenBranch(std::move(thenBranch)),
          m_elseBranch(std::move(elseBranch))
    {
    }

    [[nodiscard]] const ExpressionBase& getCondition() const
    {
        return *m_condition;
    }
    [[nodiscard]] const ExpressionBase& getThenBranch() const
    {
        return *m_thenBranch;
    }
    [[nodiscard]] const ExpressionBase& getElseBranch() const
    {
        return *m_elseBranch;
    }
    [[nodiscard]] lexer::CTokenIterator begin() const override
    {
        return m_condition->begin();
    }
    [[nodiscard]] lexer::CTokenIterator end() const override
    {
        return m_elseBranch->end();
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(ConditionalExpression)
};

class ArrayAccess final : public ExpressionBase
{
    IntrVarPtr<ExpressionBase> m_array;
    std::vector<IntrVarPtr<ExpressionBase>> m_indices;
    lexer::CTokenIterator m_endLoc;

public:
    ArrayAccess(IntrusiveVariantValue<Type> type, IntrVarPtr<ExpressionBase>&& array,
                std::vector<IntrVarPtr<ExpressionBase>>&& indices, const lexer::CTokenIterator endLoc)
        : ExpressionBase(std::in_place_type<ArrayAccess>, std::move(type), ValueCategory::Lvalue),
          m_array(std::move(array)),
          m_indices(std::move(indices)),
          m_endLoc(endLoc)
    {
    }

    [[nodiscard]] const ExpressionBase& getArray() const
    {
        return *m_array;
    }
    [[nodiscard]] const std::vector<IntrVarPtr<ExpressionBase>>& getIndices() const
    {
        return m_indices;
    }
    [[nodiscard]] lexer::CTokenIterator begin() const override
    {
        return m_array->begin();
    }
    [[nodiscard]] lexer::CTokenIterator end() const override
    {
        return m_endLoc;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(ArrayAccess)
};

class LocationAccess final : public ExpressionBase
{
    IntrVarPtr<ExpressionBase> m_wordReference;
    lexer::CTokenIterator m_beginLoc;

public:
    LocationAccess(IntrusiveVariantValue<Type> type, IntrVarPtr<ExpressionBase>&& wordReference,
                   const lexer::CTokenIterator beginLoc)
        : ExpressionBase(std::in_place_type<LocationAccess>, std::move(type), ValueCategory::Rvalue),
          m_wordReference(std::move(wordReference)),
          m_beginLoc(beginLoc)
    {
    }

    [[nodiscard]] const ExpressionBase& getWordReference() const
    {
        return *m_wordReference;
    }
    [[nodiscard]] lexer::CTokenIterator begin() const override
    {
        return m_beginLoc;
    }
    [[nodiscard]] lexer::CTokenIterator end() const override
    {
        return m_wordReference->end();
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(LocationAccess)
};

class TypeChange final : public ExpressionBase
{
    IntrVarPtr<ExpressionBase> m_expression;
    lexer::CTokenIterator m_beginLoc;

public:
    TypeChange(IntrusiveVariantValue<Type> type, IntrVarPtr<ExpressionBase>&& expression,
               const lexer::CTokenIterator beginLoc)
        : ExpressionBase(std::in_place_type<TypeChange>, std::move(type), ValueCategory::Rvalue),
          m_expression(std::move(expression)),
          m_beginLoc(beginLoc)
    {
    }

    [[nodiscard]] const ExpressionBase& getExpression() const
    {
        return *m_expression;
    }
    [[nodiscard]] lexer::CTokenIterator begin() const override
    {
        return m_beginLoc;
    }
    [[nodiscard]] lexer::CTokenIterator end() const override
    {
        return m_expression->end();
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(TypeChange)
};

class ErrorExpression final : public ExpressionBase
{
    lexer::CTokenIterator m_begin;
    lexer::CTokenIterator m_end;

public:
    ErrorExpression(IntrusiveVariantValue<Type> type, const lexer::CTokenIterator begin,
                    const lexer::CTokenIterator end)
        : ExpressionBase(std::in_place_type<ErrorExpression>, std::move(type), {}), m_begin(begin), m_end(end)
    {
    }
    [[nodiscard]] lexer::CTokenIterator begin() const override
    {
        return m_begin;
    }
    [[nodiscard]] lexer::CTokenIterator end() const override
    {
        return m_end;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(ErrorExpression)
};

// ============================================================================
// Statements
// ============================================================================

class AssignmentStatement;
class IfStatement;
class GotoStatement;
class ForStatement;
class AnswerStatement;
class CodeStatement;
class ProcedureCallStatement;
class LabelStatement;
class DummyStatement;
class BlockStatement;
class CompoundStatement;
class Statement : public AbstractIntrusiveVariant<Statement, AssignmentStatement, IfStatement, GotoStatement,
                                                  ForStatement, AnswerStatement, CodeStatement, ProcedureCallStatement,
                                                  LabelStatement, DummyStatement, BlockStatement, CompoundStatement>
{
    ScopePoint m_scopePoint;

protected:
    template <class T>
    Statement(const ScopePoint scopePoint, std::in_place_type_t<T>)
        : AbstractIntrusiveVariant(std::in_place_type<T>), m_scopePoint(scopePoint)
    {
    }

public:
    [[nodiscard]] const ScopePoint& getScopePoint() const
    {
        return m_scopePoint;
    }
};
class AssignmentStatement final : public Statement
{
    IntrVarPtr<ExpressionBase> m_target;
    IntrVarPtr<ExpressionBase> m_value;

public:
    AssignmentStatement(const ScopePoint sp, IntrVarPtr<ExpressionBase>&& target, // NOLINT(*-pro-type-member-init)
                        IntrVarPtr<ExpressionBase>&& value)                       // NOLINT(*-pro-type-member-init)
        : Statement(sp, std::in_place_type<AssignmentStatement>), m_target(std::move(target)), m_value(std::move(value))
    {
    }
    [[nodiscard]] const ExpressionBase& getExpression() const
    {
        return *m_value;
    }
    [[nodiscard]] const ExpressionBase& getTarget() const
    {
        return *m_target;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(AssignmentStatement)
};

class IfStatement final : public Statement
{
    IntrVarPtr<ExpressionBase> m_condition;
    IntrVarPtr<Statement> m_thenBranch;
    IntrVarPtr<Statement> m_elseBranch;

public:
    IfStatement(ScopePoint sp, IntrVarPtr<ExpressionBase>&& condition, // NOLINT(*-pro-type-member-init
                IntrVarPtr<Statement>&& thenBranch,                    // NOLINT(*-pro-type-member-init
                IntrVarPtr<Statement>&& elseBranch)                    // NOLINT(*-pro-type-member-init)
        : Statement(sp, std::in_place_type<IfStatement>),
          m_condition(std::move(condition)),
          m_thenBranch(std::move(thenBranch)),
          m_elseBranch(std::move(elseBranch))
    {
    }
    [[nodiscard]] const ExpressionBase& getCondition() const
    {
        return *m_condition;
    }
    [[nodiscard]] const Statement& getThenBranch() const
    {
        return *m_thenBranch;
    }
    [[nodiscard]] const Statement* getElseBranch() const
    {
        return m_elseBranch.get();
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(IfStatement)
};

class DummyStatement final : public Statement
{
public:
    explicit DummyStatement(const ScopePoint sp) // NOLINT(*-pro-type-member-init)
        : Statement(sp, std::in_place_type<DummyStatement>)
    {
    }

    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(DummyStatement)
};

class LabelStatement final : public Statement
{
    lexer::CTokenIterator m_name;
    const Statement* m_target;

public:
    LabelStatement(const lexer::CTokenIterator name, const Statement* target) // NOLINT(*-pro-type-member-init)
        : Statement(ScopePoint{0}, std::in_place_type<LabelStatement>), m_name(name), m_target(target)
    {
    }
    [[nodiscard]] const Statement* getTarget() const
    {
        return m_target;
    }

    [[nodiscard]] lexer::CTokenIterator getName() const
    {
        return m_name;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(LabelStatement)
};

class GotoStatement final : public Statement
{
    const LabelStatement* m_label;

public:
    explicit GotoStatement(const ScopePoint sp, const LabelStatement* label) // NOLINT(*-pro-type-member-init)
        : Statement(sp, std::in_place_type<GotoStatement>), m_label(label)
    {
    }
    [[nodiscard]] const LabelStatement* getLabel() const
    {
        return m_label;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(GotoStatement)
};

struct ForElement
{
    IntrVarPtr<ExpressionBase> expression;
    IntrVarPtr<ExpressionBase> step;
    IntrVarPtr<ExpressionBase> until;
    IntrVarPtr<ExpressionBase> whileCondition;
};

class ForStatement final : public Statement
{
    IntrVarPtr<ExpressionBase> m_variable;
    std::vector<ForElement> m_elements;
    IntrVarPtr<Statement> m_body;

public:
    ForStatement(ScopePoint sp, IntrVarPtr<ExpressionBase>&& variable, // NOLINT(*-pro-type-member-init
                 std::vector<ForElement>&& elements,                   // NOLINT(*-pro-type-member-init
                 IntrVarPtr<Statement>&& body)
        : Statement(sp, std::in_place_type<ForStatement>),
          m_variable(std::move(variable)),
          m_elements(std::move(elements)),
          m_body(std::move(body))
    {
    }

    [[nodiscard]] const ExpressionBase& getVariable() const
    {
        return *m_variable;
    }
    [[nodiscard]] const std::vector<ForElement>& getElements() const
    {
        return m_elements;
    }
    [[nodiscard]] const Statement& getBody() const
    {
        return *m_body;
    }

    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(ForStatement)
};

class AnswerStatement final : public Statement
{
    IntrVarPtr<ExpressionBase> m_expression;

public:
    explicit AnswerStatement(ScopePoint sp,                           // NOLINT(*-pro-type-member-init
                             IntrVarPtr<ExpressionBase>&& expression) // NOLINT(*-pro-type-member-init)
        : Statement(sp, std::in_place_type<AnswerStatement>), m_expression(std::move(expression))
    {
    }
    [[nodiscard]] const ExpressionBase& getExpression() const
    {
        return *m_expression;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(AnswerStatement)
};

class CodeStatement final : public Statement
{
    std::string m_code;

public:
    CodeStatement(const ScopePoint sp, std::string code) // NOLINT(*-pro-type-member-init)
        : Statement(sp, std::in_place_type<CodeStatement>), m_code(std::move(code))
    {
    }
    [[nodiscard]] std::string_view getCode() const
    {
        return m_code;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(CodeStatement)
};

class ProcedureCallStatement final : public Statement
{
    IntrVarPtr<ExpressionBase> m_callExpr;

public:
    explicit ProcedureCallStatement(ScopePoint sp,                         // NOLINT(*-pro-type-member-init
                                    IntrVarPtr<ExpressionBase>&& callExpr) // NOLINT(*-pro-type-member-init)
        : Statement(sp, std::in_place_type<ProcedureCallStatement>), m_callExpr(std::move(callExpr))
    {
    }
    [[nodiscard]] const ExpressionBase& getCallExpr() const
    {
        return *m_callExpr;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(ProcedureCallStatement)
};

// ============================================================================
// Declarations
// ============================================================================

class VariableDeclaration;
class ProcedureDeclaration;
class ArrayDeclaration;
class TableDeclaration;
class OverlayDeclaration;
class SwitchDeclaration;
class LabelDeclaration;
class TableElementDeclaration;

class Usable : public AbstractIntrusiveVariant<Usable, VariableDeclaration, ProcedureDeclaration, ArrayDeclaration,
                                               TableDeclaration, OverlayDeclaration, SwitchDeclaration,
                                               LabelDeclaration, TableElementDeclaration>
{
    std::vector<const Usable*> m_uses;

protected:
    template <class T>
    explicit Usable(std::in_place_type_t<T>) : AbstractIntrusiveVariant(std::in_place_type<T>)
    {
    }

public:
    [[nodiscard]] const std::vector<const Usable*>& getUses() const
    {
        return m_uses;
    }
    void setUses(std::vector<const Usable*> uses)
    {
        m_uses = std::move(uses);
    }
};

class Declaration : public Usable
{
    lexer::CTokenIterator m_nameToken;
    const syntax::Declaration* m_astNode = nullptr;

protected:
    template <class T>
    Declaration(std::in_place_type_t<T>, const lexer::CTokenIterator nameToken)
        : Usable(std::in_place_type<T>), m_nameToken(nameToken)
    {
    }

public:
    virtual ~Declaration() = default;
    [[nodiscard]] lexer::CTokenIterator getNameToken() const
    {
        return m_nameToken;
    }
    [[nodiscard]] std::string_view getName() const;

    void setAstNode(const syntax::Declaration* node)
    {
        m_astNode = node;
    }

    [[nodiscard]] const syntax::Declaration* getAstNode() const
    {
        return m_astNode;
    }
};

class VariableDeclaration final : public Declaration
{
    IntrusiveVariantValue<Type> m_type;

public:
    VariableDeclaration(const lexer::CTokenIterator nameToken, IntrusiveVariantValue<Type> type)
        : Declaration(std::in_place_type<VariableDeclaration>, nameToken), m_type(std::move(type))
    {
    }
    [[nodiscard]] const Type& getType() const
    {
        return *m_type;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(VariableDeclaration)
};

class ProcedureDeclaration final : public Declaration
{
    IntrusiveVariantValue<Type> m_type;
    std::vector<IntrVarPtr<Statement>> m_body;

public:
    ProcedureDeclaration(const lexer::CTokenIterator nameToken, IntrusiveVariantValue<Type> type)
        : Declaration(std::in_place_type<ProcedureDeclaration>, nameToken), m_type(std::move(type))
    {
    }
    [[nodiscard]] const Type& getType() const
    {
        return *m_type;
    }
    void setBody(std::vector<IntrVarPtr<Statement>> body)
    {
        m_body = std::move(body);
    }
    [[nodiscard]] const std::vector<IntrVarPtr<Statement>>& getBody() const
    {
        return m_body;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(ProcedureDeclaration)
};

class LabelDeclaration final : public Declaration
{
public:
    explicit LabelDeclaration(const lexer::CTokenIterator nameToken)
        : Declaration(std::in_place_type<LabelDeclaration>, nameToken)
    {
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(LabelDeclaration)
};

class TableElementDeclaration final : public Declaration
{
    IntrusiveVariantValue<Type> m_type;
    bool m_unsigned;
    std::optional<std::uint32_t> m_wordPosition;
    std::optional<std::uint32_t> m_bitPosition;

public:
    TableElementDeclaration(const lexer::CTokenIterator nameToken, IntrusiveVariantValue<Type> type,
                            const bool isUnsigned = false,
                            const std::optional<std::uint32_t> wordPosition = std::nullopt,
                            const std::optional<std::uint32_t> bitPosition = std::nullopt)
        : Declaration(std::in_place_type<TableElementDeclaration>, nameToken),
          m_type(std::move(type)),
          m_unsigned(isUnsigned),
          m_wordPosition(wordPosition),
          m_bitPosition(bitPosition)
    {
    }
    [[nodiscard]] const Type& getType() const
    {
        return *m_type;
    }
    [[nodiscard]] bool isUnsigned() const
    {
        return m_unsigned;
    }
    [[nodiscard]] std::optional<std::uint32_t> getWordPosition() const
    {
        return m_wordPosition;
    }
    [[nodiscard]] std::optional<std::uint32_t> getBitPosition() const
    {
        return m_bitPosition;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(TableElementDeclaration)
};

class ArrayDeclaration final : public Declaration
{
    IntrusiveVariantValue<Type> m_type;

public:
    ArrayDeclaration(const lexer::CTokenIterator nameToken, IntrusiveVariantValue<Type> type)
        : Declaration(std::in_place_type<ArrayDeclaration>, nameToken), m_type(std::move(type))
    {
    }
    [[nodiscard]] const Type& getType() const
    {
        return *m_type;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(ArrayDeclaration)
};

class TableDeclaration final : public Declaration
{
    IntrusiveVariantValue<Type> m_type;

public:
    TableDeclaration(const lexer::CTokenIterator nameToken, IntrusiveVariantValue<Type> type)
        : Declaration(std::in_place_type<TableDeclaration>, nameToken), m_type(std::move(type))
    {
    }
    [[nodiscard]] const Type& getType() const
    {
        return *m_type;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(TableDeclaration)
};

class OverlayDeclaration final : public Declaration
{
    IntrusiveVariantValue<Type> m_type;

public:
    OverlayDeclaration(const lexer::CTokenIterator nameToken, IntrusiveVariantValue<Type> type)
        : Declaration(std::in_place_type<OverlayDeclaration>, nameToken), m_type(std::move(type))
    {
    }
    [[nodiscard]] const Type& getType() const
    {
        return *m_type;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(OverlayDeclaration)
};

class SwitchDeclaration final : public Declaration
{
    std::vector<const LabelStatement*> m_labels;
    IntrusiveVariantValue<Type> m_type;

public:
    SwitchDeclaration(const lexer::CTokenIterator nameToken, std::vector<const LabelStatement*> labels)
        : Declaration(std::in_place_type<SwitchDeclaration>, nameToken),
          m_labels(std::move(labels)),
          m_type(std::in_place_type<SwitchType>)
    {
    }
    [[nodiscard]] const Type& getType() const
    {
        return *m_type;
    }
    [[nodiscard]] const std::vector<const LabelStatement*>& getLabels() const
    {
        return m_labels;
    }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(SwitchDeclaration)
};

// ============================================================================
// Helpers & Program
// ============================================================================

class BlockStatement final : public Statement
{
    std::vector<IntrVarPtr<Usable>> m_declarations;
    std::vector<IntrVarPtr<Statement>> m_statements;

public:
    BlockStatement(ScopePoint scopePoint, std::vector<IntrVarPtr<Usable>> declarations, // NOLINT(*-pro-type-member-init
                   std::vector<IntrVarPtr<Statement>> statements)
        : Statement(scopePoint, std::in_place_type<BlockStatement>),
          m_declarations(std::move(declarations)),
          m_statements(std::move(statements))
    {
    }

    [[nodiscard]] const std::vector<IntrVarPtr<Usable>>& getDeclarations() const
    {
        return m_declarations;
    }

    [[nodiscard]] const std::vector<IntrVarPtr<Statement>>& getStatements() const
    {
        return m_statements;
    }
};

class CompoundStatement final : public Statement
{
    std::vector<IntrVarPtr<Statement>> m_statements;

public:
    CompoundStatement(ScopePoint scopePoint,                         // NOLINT(*-pro-type-member-init
                      std::vector<IntrVarPtr<Statement>> statements) // NOLINT(*-pro-type-member-init
        : Statement(scopePoint, std::in_place_type<CompoundStatement>), m_statements(std::move(statements))
    {
    }

    [[nodiscard]] const std::vector<IntrVarPtr<Statement>>& getStatements() const
    {
        return m_statements;
    }
};

} // namespace coral66::semantics
