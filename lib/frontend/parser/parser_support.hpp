/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 18/03/2026
 */

#pragma once

#include <vector>

#include "lib/frontend/common/diagnostic.hpp"

/*
 * Parser support functions.
 */

namespace coral66::parser
{

/*
 * Diagnostics collection helper.
 */
class DiagnosticCollector
{
    inline static std::vector<detail::Diagnostic::DiagnosticBase> m_diagnostics;

public:
    template <typename DiagnosticType>
    static void addDiagnostic(const DiagnosticType& diagnostic)
    {
        m_diagnostics.emplace_back(diagnostic);
    }

    static std::vector<detail::Diagnostic::DiagnosticBase>& getDiagnostics()
    {
        return m_diagnostics;
    }
    static void clearDiagnostics()
    {
        m_diagnostics.clear();
    }

    template <typename DiagnosticType>
    static bool findDiagnostic(const DiagnosticType& diagnostic)
    {
        if (auto result =
                std::ranges::find_if(m_diagnostics.begin(), m_diagnostics.end(), [&diagnostic](auto const& item)
                                     { return item.getName() == diagnostic.getName(); });
            result != m_diagnostics.end())
        {
            return true;
        }
        return false;
    }
};

} // namespace coral66::parser
