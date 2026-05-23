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

rapidjson::Value ProgramDump::dump(const semantics::ResolvedIdentifiers& resolvedIdentifiers) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    return obj;
}

rapidjson::Value ProgramDump::dump(const semantics::ExpressionTypes& expressionTypes) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    return obj;
}

rapidjson::Value ProgramDump::dump(const semantics::OwnedUsables& usables) const
{
    rapidjson::Value obj(rapidjson::kObjectType);
    rapidjson::Value symbols(rapidjson::kArrayType);

    for (const auto& usablePtr : usables)
    {
        usablePtr->match(
            [&]<typename T0>(const T0& declaration)
            {
                // Ensure we are dealing with a Declaration to safely call getScopeDepth and getName
                if constexpr (std::is_base_of_v<semantics::Declaration, std::decay_t<T0>>)
                {
                    rapidjson::Value symbolObj(rapidjson::kObjectType);
                    const auto name = declaration.getName();
                    symbolObj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
                    symbolObj.AddMember("Scope Depth", declaration.getScopeDepth(), m_alloc);
                    symbols.PushBack(symbolObj, m_alloc);
                }
            });
    }

    obj.AddMember("Symbols", symbols, m_alloc);
    return obj;
}

rapidjson::Value ProgramDump::dump(const semantics::OwnedTypes& ownedTypes) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    return obj;
}

rapidjson::Value ProgramDump::dump(const semantics::TableInfo& info) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    obj.AddMember("Name", rapidjson::StringRef(info.name.c_str()), m_alloc);
    obj.AddMember("Size", info.id, m_alloc);
    obj.AddMember("Definition", dump(std::get<semantics::TableDefinition>(info.type)), m_alloc);

    return obj;
}

} // namespace coral66::tools