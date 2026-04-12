/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#include "ast_dump.hpp"

#include <rapidjson/stringbuffer.h>

namespace coral66
{

rapidjson::Value AstDump::dump(const Syntax::Expression* expression)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    if (auto* binaryExpression = dynamic_cast<const Syntax::BinaryExpression*>(expression))
    {
        obj.AddMember("NodeType", "BinaryExpression", m_alloc);
        obj.AddMember("Expression", dump(binaryExpression), m_alloc);
    }

    return obj;
}

} // namespace coral66