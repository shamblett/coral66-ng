/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 18/05/2026
 */

// #include "lib/frontend/semantics/symbol_table.hpp"

#include "program_dump.hpp"

#include <ranges>

namespace coral66::tools
{

// Entry point

rapidjson::Value ProgramDump::dump(const semantics::Program& program) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    // Symbol table
    obj.AddMember("Symbol Table", dump(program.getContext().ownedUsables), m_alloc);

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

// Helpers
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
                    symbolObj.AddMember("Name", rapidjson::StringRef(declaration.getName().data()), m_alloc);
                    symbolObj.AddMember("Scope Depth", declaration.getScopeDepth(), m_alloc);
                    symbols.PushBack(symbolObj, m_alloc);
                }
            });
    }

    obj.AddMember("Symbols", symbols, m_alloc);
    return obj;
}

rapidjson::Value ProgramDump::dump(const semantics::Usable* usable) const
{
    if (!usable)
    {
        return rapidjson::Value(rapidjson::kNullType);
    }

    return usable->match(
        [&](const semantics::VariableDeclaration& variableDeclaration)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Kind", "VariableDeclaration", m_alloc);
            obj.AddMember("Name", rapidjson::StringRef(variableDeclaration.getName().data()), m_alloc);
            // Optionally, you can dump type or uses here:
            // obj.AddMember("Type", dump(varDecl.getType()), m_alloc);
            return obj;
        },
        [&](const semantics::ProcedureDeclaration& procedureDeclaration)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Kind", "ProcedureDeclaration", m_alloc);
            obj.AddMember("Name", rapidjson::StringRef(procedureDeclaration.getName().data()), m_alloc);
            return obj;
        },
        // A generic catch-all for other declarations (ArrayDeclaration, TableDeclaration, etc.)
        [&]<typename T0>(const T0& otherDeclaration)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Kind", "OtherDeclaration", m_alloc);

            // All Usables currently derive from Declaration, so we can safely extract their name.
            if constexpr (std::is_base_of_v<semantics::Declaration, std::decay_t<T0>>)
            {
                obj.AddMember("Name", rapidjson::StringRef(otherDeclaration.getName().data()), m_alloc);
            }
            return obj;
        });
}

} // namespace coral66::tools