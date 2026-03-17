/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 17/03/2026
 */

#pragma once

#include <vector>

#include "lib/frontend/common/diagnostic.hpp"

/*
 * Preprocessor support functions.
 */

namespace coral66::preprocessor
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
};

} // namespace coral66::preprocessor
