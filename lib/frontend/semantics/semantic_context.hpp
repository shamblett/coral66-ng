/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <deque>
#include <unordered_map>
#include <vector>

#include "lib/frontend/semantics/semantics.hpp"
#include "lib/frontend/syntax/syntax.hpp"
#include "symbol_table.hpp"

namespace llvm
{
class raw_ostream;
}

namespace coral66::semantics
{

struct ParsedCode
{
    std::string llvmCode;
    std::vector<const Usable*> inputs;
    std::vector<const Usable*> outputs;
};

typedef std::vector<IntrVarPtr<Usable>> OwnedUsables;
typedef std::unordered_map<const syntax::Node*, const Usable*> ResolvedIdentifiers;
typedef std::unordered_map<const syntax::Expression*, IntrusiveVariantValue<Type>> ExpressionTypes;
typedef std::unordered_map<const syntax::CodeStatement*, ParsedCode> ParsedCodes;
typedef std::deque<IntrusiveVariantValue<Type>> OwnedTypes;
typedef std::deque<TableInfo> TableInfos;

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
    ResolvedIdentifiers resolvedIdentifiers;

    // Stores inferred types for expressions
    ExpressionTypes expressionTypes;

    // Stores parsed inline assembly for code statements
    ParsedCodes parsedCodes;

    // Owns the semantic usable objects (declarations)
    OwnedUsables ownedUsables;

    // Owns auxiliary types allocated during resolution (e.g. return types, parameter types)
    OwnedTypes ownedTypes;

    // Owns TableInfo structures
    TableInfos ownedTableInfos;

    // References to symbols that are exported via COMMON communicator
    std::vector<const Usable*> exportedSymbols;

    // The path to search for library CMI files
    std::string libraryPath;

    SemanticContext(const source_interface& sourceInterface, llvm::raw_ostream* reporterStream, bool* errorFlag,
                    std::string libPath = "")
        : source(sourceInterface), m_reporter(reporterStream), m_errors(errorFlag), libraryPath(std::move(libPath))
    {
    }

    void log(const Message& message);
};

} // namespace coral66::semantics