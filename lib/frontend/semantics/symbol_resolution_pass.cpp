/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "symbol_resolution_pass.hpp"

namespace coral66::semantics
{

SymbolResolutionPass::SymbolResolutionPass(SemanticContext& context) : m_context(context) {}

void SymbolResolutionPass::run([[maybe_unused]] const syntax::Program& program)
{
    // Walk the AST to discover all declarations, manage scopes,
    // and populate m_context.symbolTable
}

} // namespace coral66::semantics