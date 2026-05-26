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
            obj.AddMember("Type", dump(procedureDeclaration.getType()), m_alloc);
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
            obj.AddMember("Type", dump(tableElementDeclaration.getType()), m_alloc);
            if (const auto bitPos = tableElementDeclaration.getBitPosition())
            {
                obj.AddMember("BitPosition", *bitPos, m_alloc);
            }
            if (const auto wordPos = tableElementDeclaration.getWordPosition())
            {
                obj.AddMember("WordPosition", *wordPos, m_alloc);
            }
            obj.AddMember("IsUnsigned", tableElementDeclaration.isUnsigned(), m_alloc);
            return obj;
        },
        [&](const semantics::ArrayDeclaration& arrayDeclaration)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Kind", "ArrayDeclaration", m_alloc);
            const auto name = arrayDeclaration.getName();
            obj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
            obj.AddMember("Type", dump(arrayDeclaration.getType()), m_alloc);
            return obj;
        },
        [&](const semantics::TableDeclaration& tableDeclaration)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Kind", "TableDeclaration", m_alloc);
            const auto name = tableDeclaration.getName();
            obj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
            obj.AddMember("Type", dump(tableDeclaration.getType()), m_alloc);
            return obj;
        },
        [&](const semantics::OverlayDeclaration& overlayDeclaration)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Kind", "OverlayDeclaration", m_alloc);
            const auto name = overlayDeclaration.getName();
            obj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
            obj.AddMember("Type", dump(overlayDeclaration.getType()), m_alloc);
            return obj;
        },
        [&](const semantics::SwitchDeclaration& switchDeclaration)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Kind", "SwitchDeclaration", m_alloc);
            const auto name = switchDeclaration.getName();
            obj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
            obj.AddMember("Type", dump(switchDeclaration.getType()), m_alloc);
            for (const auto label : switchDeclaration.getLabels())
            {
                const auto labelName = label->getName()->getText();
                obj.AddMember("Name", rapidjson::StringRef(labelName.data(), labelName.size()), m_alloc);
                obj.AddMember("ScopePoint", label->getScopePoint().getScopeId(), m_alloc);
                obj.AddMember("TargetScopePoint", label->getTarget()->getScopePoint().getScopeId(), m_alloc);
            }
            return obj;
        });
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
            obj.AddMember("Index", primitiveType.index(), m_alloc);
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
            obj.AddMember("Index", arrayType.index(), m_alloc);
            return obj;
        },
        [&](const semantics::ProcedureType& procedureType)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("ReturnType", dump(*procedureType.getReturnType()), m_alloc);
            obj.AddMember("Index", procedureType.index(), m_alloc);
            rapidjson::Value parameters(rapidjson::kArrayType);
            for (const auto& parameter : procedureType.getParameters())
            {
                parameters.PushBack(dump(parameter), m_alloc);
            }
            obj.AddMember("Parameters", parameters, m_alloc);
            return obj;
        },
        [&](const semantics::FixedType& fixedType)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Name", "FixedType", m_alloc);
            obj.AddMember("TotalBits", fixedType.getTotalBits(), m_alloc);
            obj.AddMember("FractionalBits", fixedType.getFractionalBits(), m_alloc);
            obj.AddMember("Index", fixedType.index(), m_alloc);
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
            obj.AddMember("Index", tableType.index(), m_alloc);
            return obj;
        },
        [&](const semantics::OverlayType& overlayType)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Name", "OverlayType", m_alloc);
            obj.AddMember("Info", dump(overlayType.getInfo()), m_alloc);
            obj.AddMember("Index", overlayType.index(), m_alloc);
            return obj;
        },
        [&](const semantics::UntypedType& untypedType)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Name", "UntypedType", m_alloc);
            obj.AddMember("Index", untypedType.index(), m_alloc);
            return obj;
        },
        [&](const semantics::ErrorType& errorType)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Name", "ErrorType", m_alloc);
            obj.AddMember("Index", errorType.index(), m_alloc);
            return obj;
        },
        [&](const semantics::LabelType& labelType)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Name", "LabelType", m_alloc);
            obj.AddMember("Index", labelType.index(), m_alloc);
            return obj;
        },
        [&](const semantics::SwitchType& switchType)
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("Name", "SwitchType", m_alloc);
            obj.AddMember("Index", switchType.index(), m_alloc);
            return obj;
        });
}

rapidjson::Value ProgramDump::dump(const semantics::TableDefinition& definition) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    rapidjson::Value fields(rapidjson::kArrayType);
    for (const auto& field : definition.getFields())
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

rapidjson::Value ProgramDump::dump(const semantics::OverlayInfo& info) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    obj.AddMember("Name", rapidjson::StringRef(info.name.data(), info.name.size()), m_alloc);
    obj.AddMember("Size", info.id, m_alloc);
    obj.AddMember("Type", dump(std::get<semantics::OverlayDefinition>(info.type)), m_alloc);

    return obj;
}

rapidjson::Value ProgramDump::dump(const semantics::OverlayDefinition& definition) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    rapidjson::Value fields(rapidjson::kArrayType);
    for (const auto& field : definition.getFields())
    {
        fields.PushBack(dump(field), m_alloc);
    }
    obj.AddMember("Fields", fields, m_alloc);

    return obj;
}
} // namespace coral66::tools