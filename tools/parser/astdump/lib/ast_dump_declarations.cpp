/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#include "ast_dump.hpp"

#include <rapidjson/stringbuffer.h>

#include "lib/frontend/parser/parser_utilities.hpp"

namespace coral66
{

rapidjson::Value AstDump::dump(const Syntax::Declaration* declaration)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    if (auto* parameterDeclaration = dynamic_cast<const Syntax::ParameterDeclaration*>(declaration))
    {
        obj.AddMember("NodeType", "ParameterDeclaration", m_alloc);
        obj.AddMember("Mode", rapidjson::StringRef(parser::parameterModeToString(parameterDeclaration->getMode())),
                      m_alloc);
        obj.AddMember("Type", getTokenString(parameterDeclaration->getType().value()), m_alloc);
        rapidjson::Value identifiers(rapidjson::kArrayType);
        for (const auto& identifier : parameterDeclaration->getIdentifiers())
        {
            identifiers.PushBack(getTokenString(identifier), m_alloc);
        }
        obj.AddMember("Identifiers", identifiers, m_alloc);
    }

    if (auto* variableDeclaration = dynamic_cast<const Syntax::VariableDeclaration*>(declaration))
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
    else if (auto* procedureDeclaration = dynamic_cast<const Syntax::ProcedureDeclaration*>(declaration))
    {
        obj.AddMember("NodeType", "ProcedureDeclaration", m_alloc);
        obj.AddMember("IsRecursive", procedureDeclaration->isRecursive(), m_alloc);
        obj.AddMember("Name", getTokenString(procedureDeclaration->getName()), m_alloc);
        obj.AddMember("Type Token", getTokenString(procedureDeclaration->getTypeToken().value()), m_alloc);

        rapidjson::Value parameters(rapidjson::kArrayType);
        for (const auto& param : procedureDeclaration->getParameters())
        {
            parameters.PushBack(dump(param.get()), m_alloc);
        }
        obj.AddMember("Parameters", parameters, m_alloc);
        obj.AddMember("Body", dump(&procedureDeclaration->getBody()), m_alloc);
    }

    return obj;
}

} // namespace coral66
