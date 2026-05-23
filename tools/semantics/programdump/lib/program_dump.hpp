/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#pragma once

#include <rapidjson/document.h>

#include "lib/frontend/semantics/program.hpp"
#include "lib/frontend/semantics/semantic_context.hpp"
#include "lib/frontend/semantics/semantics.hpp"

namespace llvm
{
class Type;
}
namespace coral66::tools
{

/*
 * Dumper for the semantics program.
 *
 * Outputs the dump in JSON format.
 */
class ProgramDump
{
    rapidjson::Document::AllocatorType& m_alloc;

public:
    explicit ProgramDump(rapidjson::Document::AllocatorType& allocator) : m_alloc(allocator) {}

    // Main dump method
    rapidjson::Value dump(const semantics::Program& program) const;

    // Dump to a string
    static std::string dumpToString(const semantics::Program& program);

private:
    // Resolved Identifiers
    rapidjson::Value dump(const std::unordered_map<const syntax::Node*, const semantics::Usable*>& resolvedIdentifiers);

    // Expression types
    rapidjson::Value
        dump(std::unordered_map<const syntax::Expression*, IntrusiveVariantValue<semantics::Type>>& expressionTypes);

    // Usables
    rapidjson::Value dump(const std::vector<IntrVarPtr<semantics::Usable>>& ownedUsables);

    // Auxiliary types
    rapidjson::Value dump(const std::deque<IntrusiveVariantValue<semantics::Type>>& ownedTypes);

    // Table Information
    rapidjson::Value dump(const std::deque<semantics::TableInfo>& ownedTableInfos);

    // Helpers
    rapidjson::Value dump(const semantics::OwnedUsables& usables) const;

    rapidjson::Value dump(const semantics::Usable* usable) const;

    rapidjson::Value dump(const semantics::ResolvedIdentifiers& identifiers) const;

    rapidjson::Value dump(const semantics::Type& varType) const;

    rapidjson::Value dump(const semantics::TableInfo& info) const;

    rapidjson::Value dump(const semantics::TableDefinition& definition) const;

    rapidjson::Value dump(const semantics::Field& field) const;

    rapidjson::Value dump(const semantics::ProcedureType::Parameter& parameter) const;
};

} // namespace coral66::tools