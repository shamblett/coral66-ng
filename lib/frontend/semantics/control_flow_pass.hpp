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

class ControlFlowPass
{
    SemanticContext& m_context;

public:
    explicit ControlFlowPass(SemanticContext& context);

    // Entry point for the control flow validation pass
    void run(const Syntax::Program& program);
};

} // namespace coral66::Semantics