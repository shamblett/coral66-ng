/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "symbol_table.hpp"

#include <ranges>

namespace coral66::semantics
{

SymbolTable::SymbolTable()
{
    // Push global scope by default
    enterScope();
}

void SymbolTable::enterScope()
{
    m_scopes.emplace_back();
}

void SymbolTable::exitScope()
{
    if (!m_scopes.empty())
    {
        m_scopes.pop_back();
    }
}

bool SymbolTable::insert(const std::string_view name, const Usable* symbol)
{
    if (m_scopes.empty())
        return false;

    auto& currentScope = m_scopes.back();
    if (currentScope.contains(name))
    {
        return false; // Already exists in current scope
    }

    currentScope[name] = symbol;
    return true;
}

const Usable* SymbolTable::lookup(const std::string_view name) const
{
    for (const auto& m_scope : std::views::reverse(m_scopes))
    {
        if (auto found = m_scope.find(name); found != m_scope.end())
        {
            return found->second;
        }
    }
    return nullptr;
}

const Usable* SymbolTable::lookupInCurrentScope(const std::string_view name) const
{
    if (m_scopes.empty())
        return nullptr;

    const auto& currentScope = m_scopes.back();
    if (auto found = currentScope.find(name); found != currentScope.end())
    {
        return found->second;
    }
    return nullptr;
}

int SymbolTable::scopeCount() const
{
    return m_scopes.size();
}

} // namespace coral66::semantics