/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#include "ast_dump.hpp"

#include <rapidjson/stringbuffer.h>

namespace coral66
{

rapidjson::Value AstDump::dump(const Syntax::PlaceReference* placeReference)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    return obj;
}

} // namespace coral66
