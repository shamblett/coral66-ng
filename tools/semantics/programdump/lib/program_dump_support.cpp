/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 18/05/2026
 */

#include "program_dump.hpp"

#include <ranges>

#include "lib/frontend/semantics/semantic_utilities.hpp"

namespace coral66::tools
{

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
            const auto name = variableDeclaration.getName();
            obj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
            obj.AddMember("Type", dump(variableDeclaration.getType()), m_alloc);
            return obj;
        },
        [&](const semantics::ProcedureDeclaration& procedureDeclaration)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Kind", "ProcedureDeclaration", m_alloc);
            const auto name = procedureDeclaration.getName();
            obj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
            return obj;
        },
        [&](const semantics::LabelDeclaration& labelDeclaration)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Kind", "LabelDeclaration", m_alloc);
            const auto name = labelDeclaration.getName();
            obj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
            return obj;
        },
        [&](const semantics::TableElementDeclaration& tableElementDeclaration)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Kind", "TableElementDeclaration", m_alloc);
            const auto name = tableElementDeclaration.getName();
            obj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
            return obj;
        },
        [&](const semantics::ArrayDeclaration& arrayDeclaration)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Kind", "ArrayDeclaration", m_alloc);
            const auto name = arrayDeclaration.getName();
            obj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
            return obj;
        },
        [&](const semantics::TableDeclaration& tableDeclaration)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Kind", "TableDeclaration", m_alloc);
            const auto name = tableDeclaration.getName();
            obj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
            return obj;
        },
        [&](const semantics::OverlayDeclaration& overlayDeclaration)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Kind", "OverlayDeclaration", m_alloc);
            const auto name = overlayDeclaration.getName();
            obj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
            return obj;
        },
        [&](const semantics::SwitchDeclaration& switchDeclaration)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Kind", "SwitchDeclaration", m_alloc);
            const auto name = switchDeclaration.getName();
            obj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
            return obj;
        });
}

rapidjson::Value ProgramDump::dump(const semantics::ResolvedIdentifiers& identifiers) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    return obj;
}

rapidjson::Value ProgramDump::dump(const semantics::Type& varType) const
{
    return varType.match(
        [&](const semantics::PrimitiveType& primitiveType)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Name", "PrimitiveType", m_alloc);
            obj.AddMember("Kind", rapidjson::StringRef(semantics::primitiveTypeKindToString(primitiveType.getKind())),
                          m_alloc);
            obj.AddMember("IsLocation", primitiveType.isLocation(), m_alloc);
            obj.AddMember("IsUndefined", primitiveType.isUndefined(), m_alloc);
            obj.AddMember("IsValue", primitiveType.isValue(), m_alloc);
            return obj;
        },
        [&](const semantics::ArrayType& arrayType)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Name", "ArrayType", m_alloc);
            obj.AddMember("ElementType", dump(arrayType.getElementType()), m_alloc);
            rapidjson::Value dimensions(rapidjson::kArrayType);
            for (const auto& num : arrayType.getDimensions())
            {
                dimensions.PushBack(num, m_alloc);
            }
            obj.AddMember("Dimensions", dimensions, m_alloc);
            obj.AddMember("IsLocation", arrayType.isLocation(), m_alloc);
            obj.AddMember("IsUndefined", arrayType.isUndefined(), m_alloc);
            obj.AddMember("IsValue", arrayType.isValue(), m_alloc);
            return obj;
        },
        [&](const semantics::ProcedureType& procedureType)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("ReturnType", dump(*procedureType.getReturnType()), m_alloc);
            rapidjson::Value parameters(rapidjson::kArrayType);
            for (const auto& parameter : procedureType.getParameters())
            {
                parameters.PushBack(dump(parameter), m_alloc);
            }
            obj.AddMember("Parameters", parameters, m_alloc);
            return obj;
        },
        [&](const semantics::TableType& tableType)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Name", "TableType", m_alloc);
            obj.AddMember("TableInfo", dump(tableType.getInfo()), m_alloc);
            obj.AddMember("IsLocation", tableType.isLocation(), m_alloc);
            obj.AddMember("IsUndefined", tableType.isUndefined(), m_alloc);
            obj.AddMember("IsValue", tableType.isValue(), m_alloc);
            return obj;
        },
        [&](const auto& /*otherType*/)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Name", "UnknownType", m_alloc);
            return obj;
        });
}

rapidjson::Value ProgramDump::dump(const semantics::TableInfo& info) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    obj.AddMember("Name", rapidjson::StringRef(info.name.c_str()), m_alloc);
    obj.AddMember("Size", info.id, m_alloc);
    obj.AddMember("Definition", dump(std::get<semantics::TableDefinition>(info.type)), m_alloc);

    return obj;
}

rapidjson::Value ProgramDump::dump(const semantics::TableDefinition& definition) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    rapidjson::Value fields(rapidjson::kArrayType);
    for (const auto& field : definition.getFields() )
    {
        fields.PushBack(dump(field), m_alloc);
    }
    obj.AddMember("Fields", fields, m_alloc);
    obj.AddMember("Width", definition.getWidth(), m_alloc);
    obj.AddMember("Length", definition.getLength(), m_alloc);

    return obj;
}
rapidjson::Value ProgramDump::dump(const semantics::Field& field) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    obj.AddMember("Name", rapidjson::StringRef(field.name.c_str()), m_alloc);
    obj.AddMember("Type", dump(*field.type), m_alloc);

    return obj;
}

rapidjson::Value ProgramDump::dump(const semantics::ProcedureType::Parameter& parameter) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    obj.AddMember("Type", dump(*parameter.type), m_alloc);
    const auto name = parameter.name;
    obj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
    obj.AddMember("IsValue", parameter.isValue, m_alloc);

    return obj;

}
} // namespace coral66::tools