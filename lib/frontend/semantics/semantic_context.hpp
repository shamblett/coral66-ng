/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include "lib/frontend/common/source_interface.hpp"
#include "symbol_table.hpp"

namespace llvm
{
class raw_ostream;
}

namespace coral66::semantics
{

/*
 * Acts as the central context payload passed to each semantic pass. It currently wraps the
 * source_interface, diagnostics reporter, error flags, and an instance
 * of the SymbolTable.
 */

class SemanticContext
{
public:
    const source_interface& source;
    llvm::raw_ostream* reporter;
    bool* errors;

    SymbolTable symbolTable;

    // Future: maps to store annotations (e.g. mapping syntax::Expression nodes to their resolved semantics::Type)
    // Future: diagnostic collectors, control flow tracking state, etc.

    SemanticContext(const source_interface& sourceInterface, llvm::raw_ostream* reporterStream, bool* errorFlag)
        : source(sourceInterface), reporter(reporterStream), errors(errorFlag)
    {
    }
};

} // namespace coral66::semantics