//
// Created by steve on 16/05/2026.
//

#include "program_dump.hpp"

namespace coral66::tools {

rapidjson::Value ProgramDump::dump(const semantics::Program& program) {
    return rapidjson::Value(rapidjson::kObjectType);
}

std::string ProgramDump::dumpToString(const semantics::Program& program) {
    return "{}";
}

} // namespace coral66::tools