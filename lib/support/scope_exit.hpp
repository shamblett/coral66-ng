/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <type_traits>
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
class [[nodiscard]] ScopeExit
{
    F m_exit;
    bool m_engaged = true;

public:
    template <class Callable>
    requires std::is_constructible_v<F, Callable>
    explicit ScopeExit(Callable&& t) : m_exit(std::forward<Callable>(t)) {}

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


template <class Callable>
ScopeExit(Callable&&) -> ScopeExit<std::remove_cvref_t<Callable>>;

} // namespace coral66
