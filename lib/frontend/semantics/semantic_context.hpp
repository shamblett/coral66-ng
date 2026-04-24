/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <unordered_map>
#include <vector>

#include "lib/frontend/common/source_interface.hpp"
#include "lib/frontend/syntax/syntax.hpp"
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
    llvm::raw_ostream* m_reporter;
    bool* m_errors;
    std::size_t m_errorCount = 0;

    SymbolTable symbolTable;

    // Stores resolved symbols for identifiers in expressions
    std::unordered_map<const syntax::Node*, const Usable*> resolvedIdentifiers;

    // Owns the semantic usable objects (declarations)
    std::vector<IntrVarPtr<Usable>> ownedUsables;

    SemanticContext(const source_interface& sourceInterface, llvm::raw_ostream* reporterStream, bool* errorFlag)
        : source(sourceInterface), m_reporter(reporterStream), m_errors(errorFlag)
    {
    }

    void log(const Message& message);
};

} // namespace coral66::semantics