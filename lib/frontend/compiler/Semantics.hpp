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

#include "lib/support/vendor/tsl/ordered_map.h"

#include "lib/support/AbstractIntrusiveVariant.hpp"
#include "lib/support/IntrusiveVariantValue.hpp"

#include "lib/frontend/common/SourceObject.hpp"
#include "Lexer.hpp"
#include "lib/frontend/common/SemanticsCommon.hpp"
#include "lib/frontend/common/Attributes.hpp"


/**
 * @file Semantics.hpp
 * @brief Defines the classes and structures for the Semantic Abstract Syntax Tree (AST) for Coral 66.
 */



namespace coral66::Syntax
{
class Node;
class Program;
} // namespace coral66::Syntax


namespace coral66::Semantics
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

enum class Linkage : std::uint8_t
{
    Internal,
    External,
    None
};

enum class Lifetime : std::uint8_t
{
    Automatic,
    Static,
    Register
};

enum class InlineKind : std::uint8_t
{
    InlineDefinition,
    Inline,
    None,
};

class ScopePoint
{
    std::size_t m_scopeId;
public:
    ScopePoint(std::size_t scopeId) : m_scopeId(scopeId) {}
    [[nodiscard]] std::size_t getScopeId() const { return m_scopeId; }
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
    explicit TableDefinition(std::vector<Field> fields, std::uint64_t width, std::uint64_t length) 
        : m_fields(std::move(fields)), m_width(width), m_length(length) {}
    [[nodiscard]] const std::vector<Field>& getFields() const { return m_fields; }
    [[nodiscard]] std::uint64_t getWidth() const { return m_width; }
    [[nodiscard]] std::uint64_t getLength() const { return m_length; }
};

struct TableInfo
{
    std::string name;
    std::size_t id;
    std::variant<std::monostate, TableDefinition> type;
    template <class T>
    static T* getAttributeIf()
    { return nullptr; }
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
    { return nullptr; }
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

class ExpressionBase
    : public AbstractIntrusiveVariant<
          ExpressionBase, Constant, DeclarationRead, BinaryOperator, 
          UnaryOperator, PartWordAccess, CallExpression, ErrorExpression>
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
    [[nodiscard]] const Type& getType() const { return *m_type; }
    [[nodiscard]] ValueCategory getValueCategory() const { return m_valueCategory; }
    [[nodiscard]] bool isUndefined() const { return is<ErrorExpression>(); }
    [[nodiscard]] virtual Lexer::CTokenIterator begin() const;
    [[nodiscard]] virtual Lexer::CTokenIterator end() const;
    virtual ~ExpressionBase() = default;
};

class Constant final : public ExpressionBase
{
public:
    using Variant = std::variant<llvm::APSInt, llvm::APFloat, std::string>;

private:
    Variant m_value;
    Lexer::CTokenIterator m_begin;
    Lexer::CTokenIterator m_end;

public:
    Constant(IntrusiveVariantValue<Type> type, Variant value, Lexer::CTokenIterator begin, Lexer::CTokenIterator end)
        : ExpressionBase(std::in_place_type<Constant>, std::move(type), ValueCategory::Rvalue),
          m_value(std::move(value)), m_begin(begin), m_end(end) {}

    [[nodiscard]] const Variant& getValue() const { return m_value; }
    [[nodiscard]] Lexer::CTokenIterator begin() const override { return m_begin; }
    [[nodiscard]] Lexer::CTokenIterator end() const override { return m_end; }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(Constant)
};

class Useable;

class DeclarationRead final : public ExpressionBase
{
    const Useable* CORAL66_NON_NULL m_declRead;
    Lexer::CTokenIterator m_identifierToken;

public:
    DeclarationRead(IntrusiveVariantValue<Type> type, const Useable& useable, Lexer::CTokenIterator identifierToken)
        : ExpressionBase(std::in_place_type<DeclarationRead>, std::move(type), ValueCategory::Lvalue),
          m_declRead(&useable), m_identifierToken(identifierToken) {}

    [[nodiscard]] const Useable& getDeclRead() const { return *m_declRead; }
    [[nodiscard]] Lexer::CTokenIterator begin() const override { return m_identifierToken; }
    [[nodiscard]] Lexer::CTokenIterator end() const override { return m_identifierToken + 1; }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(DeclarationRead)
};

class BinaryOperator final : public ExpressionBase
{
    coral66::IntrVarPtr<ExpressionBase> m_leftOperand;
public:
    enum Kind
    {
        Addition, Subtraction, Multiply, Divide,
        Equal, NotEqual, LessThan, GreaterThan, LessOrEqual, GreaterOrEqual,
        LogicAnd, LogicOr
    };
private:
    Kind m_kind;
    Lexer::CTokenIterator m_operatorToken;
    coral66::IntrVarPtr<ExpressionBase> m_rightOperand;

public:
    BinaryOperator(IntrusiveVariantValue<Type> type, coral66::IntrVarPtr<ExpressionBase>&& left, Kind kind,
                   Lexer::CTokenIterator opToken, coral66::IntrVarPtr<ExpressionBase>&& right)
        : ExpressionBase(std::in_place_type<BinaryOperator>, std::move(type), ValueCategory::Rvalue),
          m_leftOperand(std::move(left)), m_kind(kind), m_operatorToken(opToken), m_rightOperand(std::move(right)) {}

    [[nodiscard]] const ExpressionBase& getLeftExpression() const { return *m_leftOperand; }
    [[nodiscard]] const ExpressionBase& getRightExpression() const { return *m_rightOperand; }
    [[nodiscard]] Kind getKind() const { return m_kind; }
    [[nodiscard]] Lexer::CTokenIterator begin() const override;
    [[nodiscard]] Lexer::CTokenIterator end() const override;
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(BinaryOperator)
};

class UnaryOperator final : public ExpressionBase
{
public:
    enum Kind { Plus, Minus, Not };
private:
    Kind m_kind;
    Lexer::CTokenIterator m_operatorToken;
    coral66::IntrVarPtr<ExpressionBase> m_operand;
public:
    UnaryOperator(IntrusiveVariantValue<Type> type, Kind kind, Lexer::CTokenIterator opToken, coral66::IntrVarPtr<ExpressionBase>&& operand)
        : ExpressionBase(std::in_place_type<UnaryOperator>, std::move(type), ValueCategory::Rvalue),
          m_kind(kind), m_operatorToken(opToken), m_operand(std::move(operand)) {}
    
    [[nodiscard]] Kind getKind() const { return m_kind; }
    [[nodiscard]] const ExpressionBase& getOperand() const { return *m_operand; }
    [[nodiscard]] Lexer::CTokenIterator begin() const override;
    [[nodiscard]] Lexer::CTokenIterator end() const override;
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(UnaryOperator)
};

class PartWordAccess final : public ExpressionBase
{
    coral66::IntrVarPtr<ExpressionBase> m_base;
    std::uint32_t m_startBit;
    std::uint32_t m_width;
    Lexer::CTokenIterator m_begin;
    Lexer::CTokenIterator m_end;
public:
    PartWordAccess(IntrusiveVariantValue<Type> type, coral66::IntrVarPtr<ExpressionBase>&& base, std::uint32_t start, std::uint32_t width,
                   Lexer::CTokenIterator beginLoc, Lexer::CTokenIterator endLoc)
        : ExpressionBase(std::in_place_type<PartWordAccess>, std::move(type), ValueCategory::Lvalue),
          m_base(std::move(base)), m_startBit(start), m_width(width), m_begin(beginLoc), m_end(endLoc) {}
    
    [[nodiscard]] Lexer::CTokenIterator begin() const override { return m_begin; }
    [[nodiscard]] Lexer::CTokenIterator end() const override { return m_end; }
    [[nodiscard]] const ExpressionBase& getBase() const { return *m_base; }
    [[nodiscard]] std::uint32_t getStartBit() const { return m_startBit; }
    [[nodiscard]] std::uint32_t getWidth() const { return m_width; }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(PartWordAccess)
};

class CallExpression final : public ExpressionBase
{
    coral66::IntrVarPtr<ExpressionBase> m_functionExpression;
    std::vector<coral66::IntrVarPtr<ExpressionBase>> m_arguments;
    Lexer::CTokenIterator m_begin;
    Lexer::CTokenIterator m_end;
public:
    CallExpression(IntrusiveVariantValue<Type> type, coral66::IntrVarPtr<ExpressionBase>&& func, std::vector<coral66::IntrVarPtr<ExpressionBase>>&& args,
                   Lexer::CTokenIterator beginLoc, Lexer::CTokenIterator endLoc)
        : ExpressionBase(std::in_place_type<CallExpression>, std::move(type), ValueCategory::Rvalue),
          m_functionExpression(std::move(func)), m_arguments(std::move(args)), m_begin(beginLoc), m_end(endLoc) {}
    
    [[nodiscard]] const ExpressionBase& getFunctionExpression() const { return *m_functionExpression; }
    [[nodiscard]] const std::vector<coral66::IntrVarPtr<ExpressionBase>>& getArguments() const { return m_arguments; }
    [[nodiscard]] Lexer::CTokenIterator begin() const override { return m_begin; }
    [[nodiscard]] Lexer::CTokenIterator end() const override { return m_end; }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(CallExpression)
};

class ErrorExpression final : public ExpressionBase
{
    Lexer::CTokenIterator m_begin;
    Lexer::CTokenIterator m_end;
public:
    ErrorExpression(IntrusiveVariantValue<Type> type, Lexer::CTokenIterator begin, Lexer::CTokenIterator end)
        : ExpressionBase(std::in_place_type<ErrorExpression>, std::move(type), {}), m_begin(begin), m_end(end) {}
    [[nodiscard]] Lexer::CTokenIterator begin() const override { return m_begin; }
    [[nodiscard]] Lexer::CTokenIterator end() const override { return m_end; }
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

class Statement : public AbstractIntrusiveVariant<Statement, AssignmentStatement, IfStatement, GotoStatement, ForStatement, AnswerStatement, CodeStatement, ProcedureCallStatement, LabelStatement>
{
    ScopePoint m_scopePoint;
protected:
    template <class T>
    Statement(ScopePoint scopePoint, std::in_place_type_t<T>) 
        : AbstractIntrusiveVariant(std::in_place_type<T>), m_scopePoint(scopePoint) {}
public:
    [[nodiscard]] const ScopePoint& getScopePoint() const { return m_scopePoint; }
};

class AssignmentStatement final : public Statement
{
    coral66::IntrVarPtr<ExpressionBase> m_target;
    coral66::IntrVarPtr<ExpressionBase> m_value;
public:
    AssignmentStatement(ScopePoint sp, coral66::IntrVarPtr<ExpressionBase>&& target, coral66::IntrVarPtr<ExpressionBase>&& value) // NOLINT(*-pro-type-member-init)
        : Statement(sp, std::in_place_type<AssignmentStatement>), m_target(std::move(target)), m_value(std::move(value)) {}
    [[nodiscard]] const ExpressionBase& getExpression() const { return *m_value; }
    [[nodiscard]] const ExpressionBase& getTarget() const { return *m_target; }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(AssignmentStatement)
};

class IfStatement final : public Statement
{
    coral66::IntrVarPtr<ExpressionBase> m_condition;
    coral66::IntrVarPtr<Statement> m_thenBranch;
    coral66::IntrVarPtr<Statement> m_elseBranch;
public:
    IfStatement(ScopePoint sp, coral66::IntrVarPtr<ExpressionBase>&& condition, coral66::IntrVarPtr<Statement>&& thenBranch, coral66::IntrVarPtr<Statement>&& elseBranch) // NOLINT(*-pro-type-member-init)
        : Statement(sp, std::in_place_type<IfStatement>), m_condition(std::move(condition)), m_thenBranch(std::move(thenBranch)), m_elseBranch(std::move(elseBranch)) {}
    [[nodiscard]] const ExpressionBase& getCondition() const { return *m_condition; }
    [[nodiscard]] const Statement& getThenBranch() const { return *m_thenBranch; }
    [[nodiscard]] const Statement* getElseBranch() const { return m_elseBranch.get(); }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(IfStatement)
};

class LabelStatement final : public Statement
{
    Lexer::CTokenIterator m_name;
    const Statement* m_target;
public:
    LabelStatement(Lexer::CTokenIterator name, const Statement* target)  // NOLINT(*-pro-type-member-init)
        : Statement(ScopePoint{0}, std::in_place_type<LabelStatement>), m_name(name), m_target(target) {}
    [[nodiscard]] const Statement* getTarget() const { return m_target; }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(LabelStatement)
};

class GotoStatement final : public Statement
{
    const LabelStatement* m_label;
public:
    explicit GotoStatement(ScopePoint sp, const LabelStatement* label) // NOLINT(*-pro-type-member-init)
        : Statement(sp, std::in_place_type<GotoStatement>), m_label(label) {}
    [[nodiscard]] const LabelStatement* getLabel() const { return m_label; }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(GotoStatement)
};

class ForStatement final : public Statement
{
    coral66::IntrVarPtr<ExpressionBase> m_variable;
    coral66::IntrVarPtr<ExpressionBase> m_start;
    coral66::IntrVarPtr<ExpressionBase> m_step;
    coral66::IntrVarPtr<ExpressionBase> m_until;
    coral66::IntrVarPtr<Statement> m_body;
public:
    ForStatement(ScopePoint sp, coral66::IntrVarPtr<ExpressionBase>&& variable, coral66::IntrVarPtr<ExpressionBase>&& start, // NOLINT(*-pro-type-member-init)
                 coral66::IntrVarPtr<ExpressionBase>&& step, coral66::IntrVarPtr<ExpressionBase>&& until,
                 coral66::IntrVarPtr<Statement>&& body)
        : Statement(sp, std::in_place_type<ForStatement>), m_variable(std::move(variable)),
          m_start(std::move(start)), m_step(std::move(step)), m_until(std::move(until)), m_body(std::move(body)) {}
    
    [[nodiscard]] const ExpressionBase& getVariable() const { return *m_variable; }
    [[nodiscard]] const ExpressionBase& getStart() const { return *m_start; }
    [[nodiscard]] const ExpressionBase& getStep() const { return *m_step; }
    [[nodiscard]] const ExpressionBase& getUntil() const { return *m_until; }
    [[nodiscard]] const Statement& getBody() const { return *m_body; }

    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(ForStatement)
};

class AnswerStatement final : public Statement
{
    coral66::IntrVarPtr<ExpressionBase> m_expression;
public:
    explicit AnswerStatement(ScopePoint sp, coral66::IntrVarPtr<ExpressionBase>&& expression) // NOLINT(*-pro-type-member-init)
        : Statement(sp, std::in_place_type<AnswerStatement>), m_expression(std::move(expression)) {}
    [[nodiscard]] const ExpressionBase& getExpression() const { return *m_expression; }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(AnswerStatement)
};

class CodeStatement final : public Statement
{
    std::string m_code;
public:
    CodeStatement(ScopePoint sp, std::string code)  // NOLINT(*-pro-type-member-init)
        : Statement(sp, std::in_place_type<CodeStatement>), m_code(std::move(code)) {}
    [[nodiscard]] std::string_view getCode() const { return m_code; }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(CodeStatement)
};

class ProcedureCallStatement final : public Statement
{
    coral66::IntrVarPtr<ExpressionBase> m_callExpr;
public:
    explicit ProcedureCallStatement(ScopePoint sp, coral66::IntrVarPtr<ExpressionBase>&& callExpr) // NOLINT(*-pro-type-member-init)
        : Statement(sp, std::in_place_type<ProcedureCallStatement>), m_callExpr(std::move(callExpr)) {}
    [[nodiscard]] const ExpressionBase& getCallExpr() const { return *m_callExpr; }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(ProcedureCallStatement)
};

// ============================================================================
// Declarations
// ============================================================================

class VariableDeclaration;
class ProcedureDeclaration;

class Useable : public AbstractIntrusiveVariant<Useable, VariableDeclaration, ProcedureDeclaration>
{
    std::vector<const Useable*> m_uses; 
protected:
    template <class T>
    Useable(std::in_place_type_t<T>) : AbstractIntrusiveVariant(std::in_place_type<T>) {}
public:
    [[nodiscard]] const std::vector<const Useable*>& getUses() const { return m_uses; }
    void setUses(std::vector<const Useable*> uses) { m_uses = std::move(uses); }
};

class Declaration : public Useable
{
    Lexer::CTokenIterator m_nameToken;
protected:
    template <class T>
    Declaration(std::in_place_type_t<T>, Lexer::CTokenIterator nameToken)
        : Useable(std::in_place_type<T>), m_nameToken(nameToken) {}
public:
    virtual ~Declaration() = default;
    [[nodiscard]] Lexer::CTokenIterator getNameToken() const { return m_nameToken; }
    [[nodiscard]] std::string_view getName() const; 
};

class VariableDeclaration final : public Declaration
{
    IntrusiveVariantValue<Type> m_type;
public:
    VariableDeclaration(Lexer::CTokenIterator nameToken, IntrusiveVariantValue<Type> type)
        : Declaration(std::in_place_type<VariableDeclaration>, nameToken), m_type(std::move(type)) {}
    [[nodiscard]] const Type& getType() const { return *m_type; }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(VariableDeclaration)
};

class ProcedureDeclaration final : public Declaration
{
    IntrusiveVariantValue<Type> m_type;
    std::vector<coral66::IntrVarPtr<Statement>> m_body;
public:
    ProcedureDeclaration(Lexer::CTokenIterator nameToken, IntrusiveVariantValue<Type> type)
        : Declaration(std::in_place_type<ProcedureDeclaration>, nameToken), m_type(std::move(type)) {}
    [[nodiscard]] const Type& getType() const { return *m_type; }
    void setBody(std::vector<coral66::IntrVarPtr<Statement>> body) { m_body = std::move(body); }
    [[nodiscard]] const std::vector<coral66::IntrVarPtr<Statement>>& getBody() const { return m_body; }
    CORAL66_GEN_INTR_VAR_WARNING_FUNCS(ProcedureDeclaration)
};

// ============================================================================
// Helpers & Program
// ============================================================================

class Program
{
    coral66::CSourceObject m_source;
    std::vector<coral66::Lexer::CToken> m_syntheticTokens;
    std::vector<coral66::IntrVarPtr<Useable>> m_globals;
public:
    Program(coral66::CSourceObject&& source, std::vector<coral66::Lexer::CToken> synthetic, std::vector<coral66::IntrVarPtr<Useable>> globals)
        : m_source(std::move(source)), m_syntheticTokens(std::move(synthetic)), m_globals(std::move(globals)) {}
    
    const std::vector<coral66::IntrVarPtr<Useable>>& getGlobals() const { return m_globals; }
    const coral66::CSourceObject& getSource() const { return m_source; }
};

Program analyse(const Syntax::Program& parseTree, CSourceObject&& cTokens,
                llvm::raw_ostream* reporter = &llvm::errs(), bool* errors = nullptr);

} // namespace coral66::Semantics
