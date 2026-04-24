/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include "lib/frontend/syntax/syntax.hpp"
#include "semantic_context.hpp"

namespace coral66::semantics
{

/*
 * Structure for the second pass. This pass expects the symbol table to be fully resolved.
 * It will infer expression types, validate function calls
 * (matching arguments), check type conversions, etc.
 */

class TypeCheckingPass
{
    SemanticContext& m_context;

public:
    explicit TypeCheckingPass(SemanticContext& context);

    // Entry point for the type checking pass
    void run(const syntax::Program& program);
};

} // namespace coral66::semantics