/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <llvm/Support/raw_ostream.h>

#include "lib/frontend/semantics/semantic_context.hpp"
#include "lib/frontend/semantics/semantics.hpp"

namespace coral66::semantics
{

class Program
{
    CSourceObject m_source;
    OwnedUsables m_globals;
    SemanticContext m_context;

public:
    Program(CSourceObject&& source, OwnedUsables globals, SemanticContext&& context)
        : m_source(std::move(source)), m_globals(std::move(globals)), m_context(std::move(context))
    {
    }

    [[nodiscard]] const std::vector<IntrVarPtr<Usable>>& getGlobals() const
    {
        return m_globals;
    }
    [[nodiscard]] const CSourceObject& getSource() const
    {
        return m_source;
    }
    [[nodiscard]] const SemanticContext& getContext() const
    {
        return m_context;
    }
};

Program analyse(const syntax::Program& parseTree, CSourceObject&& cTokens, llvm::raw_ostream* reporter = &llvm::errs(),
                bool* errors = nullptr, const std::string& libraryPath = "");

} // namespace coral66::semantics
