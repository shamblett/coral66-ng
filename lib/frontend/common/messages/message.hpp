/*
 * Project: coral66-ng
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
#include <vector>

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
public:
    struct Segment
    {
        std::string text;
        llvm::raw_ostream::Colors color = llvm::raw_ostream::SAVEDCOLOR;
        bool bold = false;
        bool bg = false;
    };

private:
    Severity m_severity = Severity::None;
    std::string m_text; // Still useful for raw text representation (e.g. without colors)
    std::vector<Segment> m_segments;

public:
    Message() = default;

    Message(const Severity severity, std::string text) : m_severity(severity), m_text(std::move(text)) {}

    Message(const Severity severity, std::vector<Segment> segments)
        : m_severity(severity), m_segments(std::move(segments))
    {
        for (const auto& segment : m_segments)
        {
            m_text += segment.text;
        }
    }

    [[nodiscard]] Severity getSeverity() const
    {
        return m_severity;
    }

    [[nodiscard]] const std::string& getText() const
    {
        return m_text;
    }

    [[nodiscard]] const std::vector<Segment>& getSegments() const
    {
        return m_segments;
    }

    friend llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const Message& message);
};

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const Message& message);

} // namespace coral66
