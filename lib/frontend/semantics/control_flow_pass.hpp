/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include "lib/frontend/syntax/syntax.hpp"
#include "semantic_context.hpp"

namespace coral66::Semantics
{

/*
 * Structure for the third pass. This checks for valid goto label boundaries, unreachable code,
 * or broken loop semantics independent of types.
 */

class ControlFlowPass
{
    SemanticContext& m_context;

public:
    explicit ControlFlowPass(SemanticContext& context);

    // Entry point for the control flow validation pass
    void run(const syntax::Program& program);
};

} // namespace coral66::Semantics