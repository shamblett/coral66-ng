/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/04/2026
 */

#pragma once

#include "diagnostic.hpp"

namespace coral66
{
/*
 * Diagnostics collector.
 * Used to collect generated diagnostic messages in a particular pass.
 */
template <typename DiagnosticType>
requires std::derived_from<DiagnosticType, detail::Diagnostic::DiagnosticBase> class DiagnosticCollection
{
    inline static std::vector<DiagnosticType> m_diagnostics;

public:
    static void addDiagnostic(const DiagnosticType& diagnostic)
    {
        m_diagnostics.emplace_back(diagnostic);
    }

    static std::vector<DiagnosticType>& getDiagnostics()
    {
        return m_diagnostics;
    }
    static void clearDiagnostics()
    {
        m_diagnostics.clear();
    }

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

} // namespace coral66