/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#include "ast_dump.hpp"

#include "lib/support/vendor/rapidjson/stringbuffer.h"

namespace coral66::tools
{

rapidjson::Value AstDump::dump(const syntax::Communicator* communicator)
{
    rapidjson::Value obj(rapidjson::kObjectType);

    if (auto* commonCommunicator = dynamic_cast<const syntax::CommonCommunicator*>(communicator))
    {
        rapidjson::Value commons(rapidjson::kArrayType);
        for (const auto& common : commonCommunicator->getItems())
        {
            commons.PushBack(dump(common.get()), m_alloc);
        }
        obj.AddMember("Common", commons, m_alloc);
    }

    if (auto* externalCommunicator = dynamic_cast<const syntax::ExternalCommunicator*>(communicator))
    {
        rapidjson::Value externals(rapidjson::kArrayType);
        for (const auto& external : externalCommunicator->getItems())
        {
            externals.PushBack(dump(external.get()), m_alloc);
        }
        obj.AddMember("External", externals, m_alloc);
    }

    if (auto* libraryCommunicator = dynamic_cast<const syntax::LibraryCommunicator*>(communicator))
    {
        rapidjson::Value libraries(rapidjson::kArrayType);
        for (const auto& library : libraryCommunicator->getItems())
        {
            libraries.PushBack(getTokenString(library), m_alloc);
        }
        obj.AddMember("Library", libraries, m_alloc);
    }

    if (auto* absoluteCommunicator = dynamic_cast<const syntax::AbsoluteCommunicator*>(communicator))
    {
        rapidjson::Value absolutes(rapidjson::kArrayType);
        for (const auto& absolute : absoluteCommunicator->getItems())
        {
            absolutes.PushBack(dump(absolute.get()), m_alloc);
        }
        obj.AddMember("Absolute", absolutes, m_alloc);
    }

    return obj;
}

} // namespace coral66::tools