/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "semantics.hpp"

#include "control_flow_pass.hpp"
#include "lib/support/vendor/ctrack/include/ctrack.hpp"
#include "semantic_context.hpp"
#include "symbol_resolution_pass.hpp"
#include "type_checking_pass.hpp"

namespace coral66::semantics
{

// Type Implementations
std::uint64_t Type::getSizeOf(const ProgramInterface& program) const
{
    return match([&](const auto& t) { return t.getSizeOfImpl(program); });
}

std::uint64_t Type::getAlignOf(const ProgramInterface& program) const
{
    return match([&](const auto& t) { return t.getAlignOfImpl(program); });
}

bool Type::operator==(const Type& rhs) const
{
    if (index() != rhs.index())
        return false;
    return match([&]<typename T0>(const T0& t) { return t.equalsImpl(rhs.as<std::decay_t<T0>>()); });
}

bool Type::operator!=(const Type& rhs) const
{
    return !(*this == rhs);
}

// PrimitiveType
// ReSharper disable once CppMemberFunctionMayBeStatic
std::uint64_t PrimitiveType::getSizeOfImpl(                 // NOLINT(*-convert-member-functions-to-static)
    [[maybe_unused]] const ProgramInterface& program) const // NOLINT(*-convert-member-functions-to-static)
{
    // Simplified: 4 bytes for Integer/Floating, 4 for Bits
    return 4;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
std::uint64_t
    PrimitiveType::getAlignOfImpl(const ProgramInterface&) const // NOLINT(*-convert-member-functions-to-static)
{
    return 4;
}

// FixedType
std::uint64_t FixedType::getSizeOfImpl(const ProgramInterface&) const
{
    if (m_totalBits <= 32)
        return 4;
    return 8;
}

std::uint64_t FixedType::getAlignOfImpl(const ProgramInterface&) const
{
    if (m_totalBits <= 32)
        return 4;
    return 8;
}

// ArrayType
std::uint64_t ArrayType::getSizeOfImpl(const ProgramInterface& program) const
{
    std::uint64_t elements = 1;
    for (const auto d : m_dimensions)
        elements *= d;
    return elements * m_elementType->getSizeOf(program);
}

std::uint64_t ArrayType::getAlignOfImpl(const ProgramInterface& program) const
{
    return m_elementType->getAlignOf(program);
}

bool ArrayType::equalsImpl(const ArrayType& rhs) const
{
    return *m_elementType == *rhs.m_elementType && m_dimensions == rhs.m_dimensions;
}

// TableType
std::uint64_t TableType::getSizeOfImpl([[maybe_unused]] const ProgramInterface& program) const
{
    if (auto* def = std::get_if<TableDefinition>(&m_info->type))
    {
        return def->getWidth() * def->getLength() * 4;
    }
    return 0;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
std::uint64_t TableType::getAlignOfImpl(                    // NOLINT(*-convert-member-functions-to-static
    [[maybe_unused]] const ProgramInterface& program) const // NOLINT(*-convert-member-functions-to-static)
{
    return 4;
}

bool TableType::equalsImpl(const TableType& rhs) const
{
    return m_info == rhs.m_info;
}

// OverlayType
// ReSharper disable once CppMemberFunctionMayBeStatic
std::uint64_t OverlayType::getSizeOfImpl(                   // NOLINT(*-convert-member-functions-to-static
    [[maybe_unused]] const ProgramInterface& program) const // NOLINT(*-convert-member-functions-to-static)
{
    return 0;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
std::uint64_t OverlayType::getAlignOfImpl(                  // NOLINT(*-convert-member-functions-to-static
    [[maybe_unused]] const ProgramInterface& program) const // NOLINT(*-convert-member-functions-to-static)
{
    return 4;
}

bool OverlayType::equalsImpl(const OverlayType& rhs) const
{
    return m_info == rhs.m_info;
}

// ProcedureType
bool ProcedureType::equalsImpl([[maybe_unused]] const ProcedureType& rhs) const
{
    if (*m_returnType != *rhs.m_returnType)
        return false;
    if (m_parameters.size() != rhs.m_parameters.size())
        return false;
    for (size_t i = 0; i < m_parameters.size(); ++i)
    {
        if (*m_parameters[i].type != *rhs.m_parameters[i].type)
            return false;
        if (m_parameters[i].isValue != rhs.m_parameters[i].isValue)
            return false;
    }
    return true;
}

// Declaration
std::string_view Declaration::getName() const
{
    if (m_nameToken->getTokenType() == lexer::TokenType::Identifier)
    {
        return std::get<std::string>(m_nameToken->getValue());
    }
    return "";
}

// Expression Base
lexer::CTokenIterator ExpressionBase::begin() const
{
    return match([](const auto& e) { return e.begin(); });
}

lexer::CTokenIterator ExpressionBase::end() const
{
    return match([](const auto& e) { return e.end(); });
}

// BinaryOperator
lexer::CTokenIterator BinaryOperator::begin() const
{
    return m_leftOperand->begin();
}

lexer::CTokenIterator BinaryOperator::end() const
{
    return m_rightOperand->end();
}

// UnaryOperator
lexer::CTokenIterator UnaryOperator::begin() const
{
    // Operator is before operand (prefix)
    return m_operatorToken;
}

lexer::CTokenIterator UnaryOperator::end() const
{
    return m_operand->end();
}

Program analyse(const syntax::Program& parseTree, CSourceObject&& cTokens, llvm::raw_ostream* reporter, bool* errors)
{
    CTRACK_NAME("Semantics");
    SemanticContext context(cTokens, reporter, errors);
    SymbolResolutionPass symPass(context);
    symPass.run(parseTree);

    TypeCheckingPass typePass(context);
    typePass.run(parseTree);

    ControlFlowPass cfPass(context);
    cfPass.run(parseTree);

    std::vector<lexer::CToken> syntheticTokens;
    std::vector<IntrVarPtr<Usable>> globals;

    // TODO We can move ownership of usable's to the program if we want, but since backend generates code from globals,
    // we return an empty globals list so the backend gracefully generates an empty module.
    return {std::move(cTokens), std::move(syntheticTokens), std::move(globals)};
}

} // namespace coral66::semantics
