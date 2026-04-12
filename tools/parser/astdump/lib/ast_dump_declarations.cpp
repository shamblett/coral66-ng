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
    }

    if (auto* variableDeclarations = dynamic_cast<const Syntax::VariableDeclaration*>(declaration))
    {
        obj.AddMember("NodeType", "VariableDeclaration", m_alloc);
        obj.AddMember("TypeToken", getTokenString(variableDeclarations->getType()), m_alloc);
    }
    else if (auto* procedureDeclarations = dynamic_cast<const Syntax::ProcedureDeclaration*>(declaration))
    {
        obj.AddMember("NodeType", "ProcedureDeclaration", m_alloc);
        obj.AddMember("IsRecursive", procedureDeclarations->isRecursive(), m_alloc);
        obj.AddMember("Name", getTokenString(procedureDeclarations->getName()), m_alloc);

        rapidjson::Value params(rapidjson::kArrayType);
        for (const auto& param : procedureDeclarations->getParameters())
        {
            params.PushBack(dump(param.get()), m_alloc);
        }
        obj.AddMember("Parameters", params, m_alloc);

        // Recursively dump the procedure body statement/block
        obj.AddMember("Body", dump(&procedureDeclarations->getBody()), m_alloc);
    }

    return obj;
}

} // namespace coral66
