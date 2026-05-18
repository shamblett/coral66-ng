/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 18/05/2026
 */

// #include "lib/frontend/semantics/symbol_table.hpp"

#include "program_dump.hpp"

namespace coral66::tools
{

// Helpers

rapidjson::Value dumpEntries(const semantics::Entries& entries)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    return obj;
}

// Entry point

rapidjson::Value ProgramDump::dump(const semantics::Program& program)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    // Symbol table
    obj.AddMember("Symbol Table", dump(program.getContext().symbolTable), m_alloc);

    return obj;
}

std::string ProgramDump::dumpToString(const semantics::Program& program)
{
    return "{}";
}

rapidjson::Value ProgramDump::dump(const semantics::SymbolTable& symbolTable)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    obj.AddMember("Scope Count", symbolTable.scopeCount(), m_alloc);
    auto entries = dumpEntries(symbolTable.getEntries());
    obj.AddMember("Entries", entries, m_alloc);
    return obj;
}

rapidjson::Value
    ProgramDump::dump(const std::unordered_map<const syntax::Node*, const semantics::Usable*>& resolvedIdentifiers)
{
    return rapidjson::Value(rapidjson::kObjectType);
}

rapidjson::Value ProgramDump::dump(
    std::unordered_map<const syntax::Expression*, IntrusiveVariantValue<semantics::Type>>& expressionTypes)
{
    return rapidjson::Value(rapidjson::kObjectType);
}

rapidjson::Value ProgramDump::dump(const std::vector<IntrVarPtr<semantics::Usable>>& ownedUsables)
{
    return rapidjson::Value(rapidjson::kObjectType);
}

rapidjson::Value ProgramDump::dump(const std::deque<IntrusiveVariantValue<semantics::Type>>& ownedTypes)
{
    return rapidjson::Value(rapidjson::kObjectType);
}

rapidjson::Value ProgramDump::dump(const std::deque<semantics::TableInfo>& ownedTableInfos)
{
    return rapidjson::Value(rapidjson::kObjectType);
}

} // namespace coral66::tools