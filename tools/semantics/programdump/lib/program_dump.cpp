/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 18/05/2026
 */

#include "program_dump.hpp"

namespace coral66::tools
{

// Entry point

rapidjson::Value ProgramDump::dump(const semantics::Program& program) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    // Symbol table
    obj.AddMember("Symbol Table", dump(program.getGlobals()), m_alloc);

    // Resolved identifiers
    obj.AddMember("Resolved Identifiers", dump(program.getContext().resolvedIdentifiers), m_alloc);

    return obj;
}

std::string ProgramDump::dumpToString(const semantics::Program& program)
{
    return "{}";
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