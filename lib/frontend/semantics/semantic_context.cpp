/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "semantic_context.hpp"

namespace coral66::semantics
{

void SemanticContext::log(const Message& message)
{
    if (message.getSeverity() == Severity::Error)
    {
        m_errorCount++;
        if (m_errors) *m_errors = true;
    }
    if (m_reporter)
    {
        *m_reporter << message;
    }
}
} // namespace coral66::semantics