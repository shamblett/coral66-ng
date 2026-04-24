/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "control_flow_pass.hpp"

namespace coral66::semantics
{

ControlFlowPass::ControlFlowPass(SemanticContext& context) : m_context(context) {}

void ControlFlowPass::run([[maybe_unused]] const syntax::Program& program)
{
    // Walk the AST to validate GOTO statements, labels, loop bounds,
    // and overall procedure return paths.
}

} // namespace coral66::semantics