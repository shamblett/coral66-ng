/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "type_checking_pass.hpp"

namespace coral66::Semantics
{

TypeCheckingPass::TypeCheckingPass(SemanticContext& context) : m_context(context) {}

void TypeCheckingPass::run([[maybe_unused]] const syntax::Program& program)
{
    // Walk the AST to validate type rules, expression evaluations,
    // assignments, parameter passing, and annotate AST nodes with type information
}

} // namespace coral66::Semantics