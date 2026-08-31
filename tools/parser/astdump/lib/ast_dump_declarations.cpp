/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#include "ast_dump.hpp"

#include "lib/frontend/parser/parser_utilities.hpp"
#include "lib/support/vendor/rapidjson/stringbuffer.h"

namespace coral66::tools
{

rapidjson::Value AstDump::dump(const Declaration* declaration)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    if (auto* parameterDeclaration = dynamic_cast<const ParameterDeclaration*>(declaration))
    {
        obj.AddMember("NodeType", "ParameterDeclaration", m_alloc);
        obj.AddMember("Mode", rapidjson::StringRef(parser::parameterModeToString(parameterDeclaration->getMode())),
                      m_alloc);
        if (parameterDeclaration->getType().has_value())
            obj.AddMember("Type", getTokenString(parameterDeclaration->getType().value()), m_alloc);

        rapidjson::Value identifiers(rapidjson::kArrayType);
        for (const auto& identifier : parameterDeclaration->getIdentifiers())
        {
            identifiers.PushBack(getTokenString(identifier), m_alloc);
        }
        obj.AddMember("Identifiers", identifiers, m_alloc);
    }
    else if (auto* variableDeclaration = dynamic_cast<const VariableDeclaration*>(declaration))
    {
        obj.AddMember("NodeType", "VariableDeclaration", m_alloc);
        obj.AddMember("Type", getTokenString(variableDeclaration->getType()), m_alloc);

        rapidjson::Value identifiers(rapidjson::kArrayType);
        for (const auto& identifier : variableDeclaration->getIdentifiers())
        {
            identifiers.PushBack(getTokenString(identifier), m_alloc);
        }
        obj.AddMember("Identifiers", identifiers, m_alloc);
        obj.AddMember("Preset", variableDeclaration->hasPreset(), m_alloc);

        rapidjson::Value presetTokens(rapidjson::kArrayType);
        for (const auto& presetToken : variableDeclaration->getPresetTokens())
        {
            presetTokens.PushBack(getTokenString(presetToken), m_alloc);
        }
        obj.AddMember("Preset Tokens", presetTokens, m_alloc);
    }
    else if (auto* procedureDeclaration = dynamic_cast<const ProcedureDeclaration*>(declaration))
    {
        obj.AddMember("NodeType", "ProcedureDeclaration", m_alloc);
        obj.AddMember("IsRecursive", procedureDeclaration->isRecursive(), m_alloc);
        obj.AddMember("Name", getTokenString(procedureDeclaration->getName()), m_alloc);
        if (procedureDeclaration->getTypeToken().has_value())
        {
            obj.AddMember("Type Token", getTokenString(procedureDeclaration->getTypeToken().value()), m_alloc);
        }

        rapidjson::Value parameters(rapidjson::kArrayType);
        for (const auto& param : procedureDeclaration->getParameters())
        {
            parameters.PushBack(dump(param.get()), m_alloc);
        }
        obj.AddMember("Parameters", parameters, m_alloc);
        obj.AddMember("Body", dump(&procedureDeclaration->getBody()), m_alloc);
    }
    else if (auto* arrayDeclaration = dynamic_cast<const ArrayDeclaration*>(declaration))
    {
        obj.AddMember("NodeType", "ArrayDeclaration", m_alloc);
        obj.AddMember("Type", getTokenString(arrayDeclaration->getType()), m_alloc);

        rapidjson::Value identifiers(rapidjson::kArrayType);
        for (const auto& identifier : arrayDeclaration->getIdentifiers())
        {
            identifiers.PushBack(getTokenString(identifier), m_alloc);
        }
        obj.AddMember("Identifiers", identifiers, m_alloc);

        rapidjson::Value dimensions(rapidjson::kArrayType);
        rapidjson::Value dimensionValues(rapidjson::kArrayType);
        for (const auto& [fst, snd] : arrayDeclaration->getDimensions())
        {
            dimensionValues.PushBack(dump(fst.get()), m_alloc);
            dimensionValues.PushBack(dump(snd.get()), m_alloc);
        }
        dimensions.PushBack(dimensionValues, m_alloc);
        obj.AddMember("Dimensions", dimensions, m_alloc);
        obj.AddMember("Has Preset", arrayDeclaration->hasPreset(), m_alloc);

        rapidjson::Value presetTokens(rapidjson::kArrayType);
        for (const auto& presetToken : arrayDeclaration->getPresetTokens())
        {
            presetTokens.PushBack(getTokenString(presetToken), m_alloc);
        }
        obj.AddMember("Preset Tokens", presetTokens, m_alloc);
    }
    else if (auto* tableElementDeclaration = dynamic_cast<const TableElementDeclaration*>(declaration))
    {
        obj.AddMember("NodeType", "TableElementDeclaration", m_alloc);
        obj.AddMember("Identifier", getTokenString(tableElementDeclaration->getIdentifier()), m_alloc);
        if (tableElementDeclaration->getType().has_value())
            obj.AddMember("Type", getTokenString(tableElementDeclaration->getType().value()), m_alloc);
        obj.AddMember("Is Unsigned", tableElementDeclaration->isUnsigned(), m_alloc);
        if (tableElementDeclaration->getWordPosition().has_value())
            obj.AddMember("Word Position", getTokenString(tableElementDeclaration->getWordPosition().value()), m_alloc);
        if (tableElementDeclaration->getBitPosition().has_value())
            obj.AddMember("Bit Position", getTokenString(tableElementDeclaration->getBitPosition().value()), m_alloc);
    }

    else if (auto* tableDeclaration = dynamic_cast<const TableDeclaration*>(declaration))
    {
        obj.AddMember("NodeType", "TableDeclaration", m_alloc);
        obj.AddMember("Identifier", getTokenString(tableDeclaration->getIdentifier()), m_alloc);
        obj.AddMember("Width", dump(tableDeclaration->getWidth()), m_alloc);
        obj.AddMember("Length", dump(tableDeclaration->getLength()), m_alloc);

        rapidjson::Value elements(rapidjson::kArrayType);
        for (const auto& element : tableDeclaration->getElements())
        {
            elements.PushBack(dump(element.get()), m_alloc);
        }
        obj.AddMember("Elements", elements, m_alloc);

        obj.AddMember("Has Preset", tableDeclaration->hasPreset(), m_alloc);

        rapidjson::Value presetTokens(rapidjson::kArrayType);
        for (const auto& presetToken : tableDeclaration->getPresetTokens())
        {
            presetTokens.PushBack(getTokenString(presetToken), m_alloc);
        }
        obj.AddMember("Preset Tokens", presetTokens, m_alloc);
    }
    else if (auto* overlayDeclaration = dynamic_cast<const OverlayDeclaration*>(declaration))
    {
        obj.AddMember("NodeType", "OverlayDeclaration", m_alloc);
        obj.AddMember("Base", getTokenString(overlayDeclaration->getBase()), m_alloc);
        obj.AddMember("Offset", getTokenString(overlayDeclaration->getOffset()), m_alloc);
        if (overlayDeclaration->getOffset2().has_value())
            obj.AddMember("Offset2", getTokenString(overlayDeclaration->getOffset2().value()), m_alloc);
    }
    else if (auto* placeSpecification = dynamic_cast<const PlaceSpecification*>(declaration))
    {
        obj.AddMember("NodeType", "PlaceSpecification", m_alloc);
        rapidjson::Value identifiers(rapidjson::kArrayType);
        for (const auto& identifier : placeSpecification->getIdentifiers())
        {
            identifiers.PushBack(getTokenString(identifier), m_alloc);
        }
        obj.AddMember("Identifiers", identifiers, m_alloc);
    }
    else if (auto* procedureSpecification = dynamic_cast<const ProcedureSpecification*>(declaration))
    {
        obj.AddMember("NodeType", "ProcedureSpecification", m_alloc);
        if (procedureSpecification->getType().has_value())
            obj.AddMember("Type", getTokenString(procedureSpecification->getType().value()), m_alloc);
        obj.AddMember("Name", getTokenString(procedureSpecification->getName()), m_alloc);

        rapidjson::Value parameters(rapidjson::kArrayType);
        for (const auto& parameter : procedureSpecification->getParameters())
        {
            parameters.PushBack(dump(parameter.get()), m_alloc);
        }
        obj.AddMember("Parameters", parameters, m_alloc);
    }

    else if (auto* switchDeclaration = dynamic_cast<const SwitchDeclaration*>(declaration))
    {
        obj.AddMember("NodeType", "SwitchDeclaration", m_alloc);
        obj.AddMember("Name", getTokenString(switchDeclaration->getIdentifier()), m_alloc);

        rapidjson::Value labels(rapidjson::kArrayType);
        for (const auto& l : switchDeclaration->getLabels())
        {
            labels.PushBack(getTokenString(l), m_alloc);
        }
        obj.AddMember("Labels", labels, m_alloc);
    }

    return obj;
}

} // namespace coral66::tools
