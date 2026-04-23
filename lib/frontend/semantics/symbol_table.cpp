/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "symbol_table.hpp"

namespace coral66::Semantics
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

bool SymbolTable::insert(std::string_view name, const Usable* symbol)
{
    if (m_scopes.empty())
        return false;

    auto& currentScope = m_scopes.back();
    if (currentScope.find(name) != currentScope.end())
    {
        return false; // Already exists in current scope
    }

    currentScope[name] = symbol;
    return true;
}

const Usable* SymbolTable::lookup(std::string_view name) const
{
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it)
    {
        if (auto found = it->find(name); found != it->end())
        {
            return found->second;
        }
    }
    return nullptr;
}

const Usable* SymbolTable::lookupInCurrentScope(std::string_view name) const
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

} // namespace coral66::Semantics