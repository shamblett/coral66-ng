/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 18/05/2026
 */

#include "program_dump.hpp"

#include "lib/frontend/semantics/semantic_support.hpp"
#include "lib/support/text.hpp"

namespace coral66::tools
{

static std::string getTokenString(const lexer::CTokenIterator& token)
{
    if (!token)
    {
        return "";
    }

    std::string tokenValueStr;
    std::visit(
        [&]<typename T0>(const T0& arg)
        {
            using T = std::decay_t<T0>;

            if constexpr (std::is_same_v<T, std::string>)
            {
                tokenValueStr = arg;
            }
            else if constexpr (std::is_same_v<T, coral66::lexer::NonCharString>)
            {
                tokenValueStr = "<NonCharString>";
            }
            else if constexpr (std::is_same_v<T, llvm::APSInt>)
            {
                tokenValueStr = coral66::to_string(arg);
            }
            else if constexpr (std::is_same_v<T, llvm::APFloat>)
            {
                tokenValueStr = coral66::to_string(arg);
            }
            else if constexpr (std::is_same_v<T, std::monostate>)
            {
                tokenValueStr = "";
            }
        },
        token->getValue());

    return tokenValueStr;
}

// Entry point

rapidjson::Value ProgramDump::dump(const semantics::Program& program, const std::string& reports)
{
    m_sourceInterface = &program.getSource();
    rapidjson::Value obj(rapidjson::kObjectType);

    // Symbol table
    obj.AddMember("Symbol Table", dump(program.getGlobals()), m_alloc);

    // Resolved identifiers
    obj.AddMember("Resolved Identifiers", dump(program.getContext().resolvedIdentifiers), m_alloc);

    // Expression Types
    obj.AddMember("Expression Types", dump(program.getContext().expressionTypes), m_alloc);

    // Auxiliary types
    obj.AddMember("Auxiliary Types", dump(program.getContext().ownedTypes), m_alloc);

    // Table Information
    obj.AddMember("Table Info", dump(program.getContext().ownedTableInfos), m_alloc);

    // Diagnostics
    obj.AddMember("Diagnostics", dump(program.getContext(), reports), m_alloc);

    return obj;
}

std::string ProgramDump::dumpToString(const semantics::Program& program, const std::string& reports)
{
    return "{}";
}

rapidjson::Value ProgramDump::dump(const semantics::ResolvedIdentifiers& resolvedIdentifiers) const
{
    rapidjson::Value arr(rapidjson::kArrayType);

    for (const auto& [node, usable] : resolvedIdentifiers)
    {
        rapidjson::Value item(rapidjson::kObjectType);
        std::string text = getTokenString(node->begin());
        item.AddMember("NodeText", rapidjson::Value(text.c_str(), text.size(), m_alloc), m_alloc);

        if (node->begin() && m_sourceInterface)
        {
            rapidjson::Value location(rapidjson::kObjectType);
            const auto fileId = node->begin()->getFileId();
            const auto offset = node->begin()->getOffset();
            if (fileId < m_sourceInterface->getFiles().size())
            {
                const auto line = m_sourceInterface->getLineNumber(fileId, offset);
                const auto col = offset - m_sourceInterface->getLineStartOffset(fileId, line) + 1;
                location.AddMember("FileId", fileId, m_alloc);
                location.AddMember("Line", line, m_alloc);
                location.AddMember("Column", col, m_alloc);
            }
            else
            {
                location.AddMember("Offset", offset, m_alloc);
            }
            item.AddMember("Location", location, m_alloc);
        }

        item.AddMember("Usable", dump(usable), m_alloc);
        arr.PushBack(item, m_alloc);
    }

    return arr;
}

rapidjson::Value ProgramDump::dump(const semantics::ExpressionTypes& expressionTypes) const
{
    rapidjson::Value arr(rapidjson::kArrayType);

    for (const auto& [expression, type] : expressionTypes)
    {
        rapidjson::Value item(rapidjson::kObjectType);
        std::string text = getTokenString(expression->begin());
        item.AddMember("ExpressionText", rapidjson::Value(text.c_str(), text.size(), m_alloc), m_alloc);

        if (expression->begin() && m_sourceInterface)
        {
            rapidjson::Value location(rapidjson::kObjectType);
            const auto fileId = expression->begin()->getFileId();
            const auto offset = expression->begin()->getOffset();
            if (fileId < m_sourceInterface->getFiles().size())
            {
                const auto line = m_sourceInterface->getLineNumber(fileId, offset);
                const auto col = offset - m_sourceInterface->getLineStartOffset(fileId, line) + 1;
                location.AddMember("FileId", fileId, m_alloc);
                location.AddMember("Line", line, m_alloc);
                location.AddMember("Column", col, m_alloc);
            }
            else
            {
                location.AddMember("Offset", offset, m_alloc);
            }
            item.AddMember("Location", location, m_alloc);
        }

        item.AddMember("Type", dump(*type), m_alloc);
        arr.PushBack(item, m_alloc);
    }

    return arr;
}

rapidjson::Value ProgramDump::dump(const semantics::OwnedUsables& ownedUsables) const
{
    rapidjson::Value obj(rapidjson::kObjectType);
    rapidjson::Value symbols(rapidjson::kArrayType);

    for (const auto& usablePtr : ownedUsables)
    {
        usablePtr->match(
            [&]<typename T0>(const T0& declaration)
            {
                // Ensure we are dealing with a Declaration to safely call getScopeDepth and getName
                if constexpr (std::is_base_of_v<semantics::Declaration, std::decay_t<T0>>)
                {
                    rapidjson::Value symbolObj(rapidjson::kObjectType);
                    const auto name = declaration.getName();
                    symbolObj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
                    symbolObj.AddMember("Scope Depth", declaration.getScopeDepth(), m_alloc);
                    symbols.PushBack(symbolObj, m_alloc);
                }
            });
    }

    obj.AddMember("Symbols", symbols, m_alloc);
    return obj;
}

rapidjson::Value ProgramDump::dump(const semantics::OwnedTypes& ownedTypes) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    rapidjson::Value types(rapidjson::kArrayType);
    for (const auto& type : ownedTypes)
    {
        types.PushBack(dump(*type), m_alloc);
    }
    obj.AddMember("Types", types, m_alloc);

    return obj;
}

rapidjson::Value ProgramDump::dump(const semantics::TableInfos& info) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    rapidjson::Value infos(rapidjson::kArrayType);
    for (const auto& tableInfo : info)
    {
        infos.PushBack(dump(tableInfo), m_alloc);
    }
    obj.AddMember("Infos", infos, m_alloc);

    return obj;
}

rapidjson::Value ProgramDump::dump(const semantics::SemanticContext& context, const std::string& reports) const
{
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("ErrorCount", context.m_errorCount, m_alloc);
    if (const auto diagnostics = semantics::DiagnosticCollector::getDiagnostics(); !diagnostics.empty())
    {
        for (const auto& diagnostic : diagnostics)
        {
            const auto name = diagnostic.getName();
            obj.AddMember("Name", rapidjson::StringRef(name.data(), name.size()), m_alloc);
            auto severity = "";
            switch (diagnostic.getSeverity())
            {
                case Severity::Error: severity = "Error"; break;
                case Severity::Warning: severity = "Warning"; break;
                case Severity::Note: severity = "Note"; break;
                case Severity::None: severity = "None"; break;
            }
            obj.AddMember("severity", rapidjson::StringRef(severity), m_alloc);
        }
        obj.AddMember("Reports", rapidjson::StringRef(reports.c_str()), m_alloc);
    }

    return obj;
}

} // namespace coral66::tools