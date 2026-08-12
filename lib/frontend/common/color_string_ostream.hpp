/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 05/04/2026
 */

#pragma once
#include <llvm/Support/raw_ostream.h>

#include <string>
#include <vector>

#include "messages/message.hpp"

namespace coral66::detail
{

/**
 *
 * LLVM raw_ostream override for building Message Segments
 *
 */
class ColorStringOstream : public llvm::raw_ostream
{
    std::vector<Message::Segment>& m_segments;
    llvm::raw_ostream::Colors m_currentColor = llvm::raw_ostream::SAVEDCOLOR;
    bool m_currentBold = false;
    bool m_currentBg = false;
    uint64_t m_pos = 0;

public:
    explicit ColorStringOstream(std::vector<Message::Segment>& segments) : m_segments(segments) {}

    ~ColorStringOstream() override
    {
        flush();
    }

    // By default, llvm::raw_ostream::has_colors() returns false.
    // llvm::WithColor checks this, and if it's true, it calls changeColor().
    // We *always* want ColorStringOstream to capture colors into segments.
    // The actual stream that prints the Message will decide whether to emit them.
    [[nodiscard]] bool has_colors() const override
    {
        return true;
    } // To let WithColor use color

    raw_ostream& changeColor(const Colors Color, const bool Bold = false, const bool BG = false) override
    { // NOLINT(*-default-arguments)
        m_currentColor = Color;
        m_currentBold = Bold;
        m_currentBg = BG;
        return *this;
    }

    raw_ostream& resetColor() override
    {
        m_currentColor = llvm::raw_ostream::SAVEDCOLOR;
        m_currentBold = false;
        m_currentBg = false;
        return *this;
    }

    raw_ostream& reverseColor() override
    {
        return *this;
    }

    void write_impl(const char* Ptr, size_t Size) override
    {
        if (Size == 0)
            return;

        std::string text(Ptr, Size);
        if (!m_segments.empty() && m_segments.back().color == m_currentColor && m_segments.back().bold == m_currentBold
            && m_segments.back().bg == m_currentBg)
        {
            m_segments.back().text += text;
        }
        else
        {
            m_segments.push_back({std::move(text), m_currentColor, m_currentBold, m_currentBg});
        }
        m_pos += Size;
    }

    uint64_t current_pos() const override
    {
        return m_pos;
    }
};

} // namespace coral66::detail
