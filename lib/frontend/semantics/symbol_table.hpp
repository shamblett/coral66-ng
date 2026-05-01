/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>

#include "semantics.hpp"

/*
 * Provides a SymbolTable class that manages stacked/nested scopes to correctly resolve
 * variables and procedures within their respective blocks.
 */

namespace coral66::semantics
{

class SymbolTable
{
    std::vector<std::unordered_map<std::string_view, const Usable*>> m_scopes;

public:
    SymbolTable();

    void enterScope();
    void exitScope();
    bool insert(std::string_view name, const Usable* symbol);
    [[nodiscard]] const Usable* lookup(std::string_view name) const;
    [[nodiscard]] const Usable* lookupInCurrentScope(std::string_view name) const;
    [[nodiscard]] int lookupScopeDepth(std::string_view name) const;
    [[nodiscard]] int scopeCount() const;
};

} // namespace coral66::semantics
