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

    if (auto* switchPlaceReference = dynamic_cast<const Syntax::SwitchPlaceReference*>(placeReference))
    {
        obj.AddMember("NodeType", "SwitchPlaceReference", m_alloc);
        obj.AddMember("name", getTokenString(switchPlaceReference->getName()), m_alloc);

        rapidjson::Value labels(rapidjson::kArrayType);
        for (const auto& label : switchPlaceReference->getLabels())
        {
            labels.PushBack(getTokenString(label), m_alloc);
        }
        obj.AddMember("Labels", labels, m_alloc);
    }

    return obj;
}

} // namespace coral66
