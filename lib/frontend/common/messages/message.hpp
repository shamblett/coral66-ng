/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

/**
 * Base  message definition
 *
 */

#include <llvm/Support/raw_ostream.h>

#include <string>

namespace coral66
{
enum class Severity
{
    None,
    Error,
    Warning,
    Note
};

class Message final
{
    Severity m_severity = Severity::None;
    std::string m_text;

public:
    Message() = default;

    Message(const Severity severity, std::string text) : m_severity(severity), m_text(std::move(text)) {}

    [[nodiscard]] Severity getSeverity() const
    {
        return m_severity;
    }

    [[nodiscard]] const std::string& getText() const
    {
        return m_text;
    }

    friend llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const Message& message);
};

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const Message& message);

} // namespace coral66
