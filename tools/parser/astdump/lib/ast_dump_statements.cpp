/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#include "ast_dump.hpp"

#include <rapidjson/stringbuffer.h>

namespace coral66
{

rapidjson::Value AstDump::dump(const Syntax::Statement* statement)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    if (auto* assignmentStatement = dynamic_cast<const Syntax::AssignmentStatement*>(statement))
    {
        obj.AddMember("NodeType", "AssignmentStatement", m_alloc);
        obj.AddMember("LHS", dump(&assignmentStatement->getVariable()), m_alloc);
        obj.AddMember("RHS", dump(&assignmentStatement->getExpression()), m_alloc);
    }
    return obj;
}

} // namespace coral66
