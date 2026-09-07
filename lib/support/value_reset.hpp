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
 * A RAII wrapper that assigns a specific value to a variable when it goes out of scope.
 *
 * ValueReset holds a reference to a variable and a value. Upon destruction,
 * it assigns the stored value to the referenced variable. This is useful for
 * ensuring a variable is set to a specific state (e.g., resetting a flag to false,
 * or restoring a default value) at the end of a block.
 *
 * @tparam T The type of the variable to be reset.
 */
template <class T>
class [[nodiscard]] ValueReset
{
    T m_valueAfter;
    T* m_assignedTo;

public:
    template <class U>
    requires std::is_constructible_v<T, U>
    explicit ValueReset(T& assignedTo, U&& valueAfter)
        : m_valueAfter(std::forward<U>(valueAfter)), m_assignedTo(&assignedTo)
    {
    }

    ~ValueReset()
    {
        if (m_assignedTo)
        {
            *m_assignedTo = std::move(m_valueAfter);
        }
    }

    ValueReset(const ValueReset&) = delete;
    ValueReset& operator=(const ValueReset&) = delete;

    ValueReset(ValueReset&& rhs) noexcept
        : m_valueAfter(std::move(rhs.m_valueAfter)), 
          m_assignedTo(std::exchange(rhs.m_assignedTo, nullptr))
    {
    }

    ValueReset& operator=(ValueReset&& rhs) noexcept
    {
        m_assignedTo = std::exchange(rhs.m_assignedTo, nullptr);
        m_valueAfter = std::move(rhs.m_valueAfter);
        return *this;
    }
};


template <class T, class U>
ValueReset(T&, U&&) -> ValueReset<T>;

} // namespace coral66
