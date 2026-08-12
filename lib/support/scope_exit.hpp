/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <utility>

namespace coral66
{
/**
 * An RAII wrapper that executes a callable when it goes out of scope.
 *
 * ScopeExit is used to guarantee that clean-up code or state restoration logic
 * runs when a block is exited, regardless of how it exits (e.g., return, exception).
 * This pattern is often referred to as a "scope guard".
 *
 * @tparam F The type of the callable to execute.
 */
template <class F>
class ScopeExit
{
    F m_exit;
    bool m_engaged = true;

public:
    explicit ScopeExit(F&& t) : m_exit(std::move(t)) {}

    ~ScopeExit()
    {
        if (m_engaged)
        {
            m_exit();
        }
    }

    ScopeExit(const ScopeExit&) = delete;

    ScopeExit(ScopeExit&& rhs) noexcept : m_exit(std::move(rhs.m_exit)), m_engaged(std::exchange(rhs.m_engaged, false))
    {
    }

    ScopeExit& operator=(const ScopeExit&) = delete;

    ScopeExit& operator=(ScopeExit&& rhs) noexcept
    {
        m_exit = std::move(rhs.m_exit);
        m_engaged = std::exchange(rhs.m_engaged, false);
        return *this;
    }
};
} // namespace coral66
