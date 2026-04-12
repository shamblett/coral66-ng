/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#include "ast_dump.hpp"

#include <rapidjson/stringbuffer.h>

namespace coral66
{

rapidjson::Value AstDump::dump(const Syntax::ProcedureCallStatement* callStatement)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    return obj;
}

rapidjson::Value AstDump::dump(const Syntax::ProcedureDeclaration* declaration)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    return obj;
}

rapidjson::Value AstDump::dump(const Syntax::ProcedureSpecification* specification)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    return obj;
}

} // namespace coral66
