/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

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
class ValueReset
{
    T m_valueAfter;
    T* m_assignedTo;

public:
    template <class U = T>
    ValueReset(T& assignedTo, U valueAfter) : m_valueAfter(valueAfter), m_assignedTo(&assignedTo)
    {
    }

    ~ValueReset()
    {
        if (m_assignedTo)
        {
            *m_assignedTo = m_valueAfter;
        }
    }

    ValueReset(const ValueReset&) = delete;
    ValueReset& operator=(const ValueReset&) = delete;

    ValueReset(ValueReset&& rhs) noexcept
    {
        m_assignedTo = std::exchange(rhs.m_assignedTo, nullptr);
        m_valueAfter = rhs.m_valueAfter;
    }

    ValueReset& operator=(ValueReset&& rhs) noexcept
    {
        m_assignedTo = std::exchange(rhs.m_assignedTo, nullptr);
        m_valueAfter = rhs.m_valueAfter;
        return *this;
    }
};
} // namespace coral66
