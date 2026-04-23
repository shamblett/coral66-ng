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
* Structure for the first pass. This pass will walk the Syntax::Program,
* establish scopes in the symbol table, and resolve/bind identifiers to their
* declarations without worrying about expression type rules.
*/

class SymbolResolutionPass
{
    SemanticContext& m_context;

public:
    explicit SymbolResolutionPass(SemanticContext& context);

    // Entry point for the symbol resolution pass
    void run(const Syntax::Program& program);
};

} // namespace coral66::Semantics