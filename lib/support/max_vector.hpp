/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <algorithm>
#include <array>

#include "constexpr.hpp"

namespace coral66
{
/**
 * A fixed-capacity, stack-allocated vector implementation compatible with constexpr.
 *
 * MaxVector mimics the interface of std::vector but allocates its storage statically
 * within the object itself (using std::array). This allows it to be used in
 * constexpr contexts where dynamic memory allocation (std::vector) is historically
 * restricted or impossible.
 *
 * It is needed in coral66 to support compile-time string processing, diagnostic generation,
 * and command-line parsing without relying on heap allocations.
 *
 * @tparam T The type of elements.
 * @tparam N The maximum capacity of the vector.
 */

template <class T, std::size_t N>
class MaxVector
{
    std::array<T, N> m_array{};
    suitableUInt<N> m_size{};

public:
    constexpr MaxVector() = default;

    explicit constexpr MaxVector(std::size_t size) : m_size(size)
    {
        CORAL66_ASSERT(m_size <= N);
    }

    constexpr MaxVector(std::initializer_list<T> initializerList) noexcept : m_size(initializerList.size())
    {
        CORAL66_ASSERT(m_size <= N);
        std::copy(initializerList.begin(), initializerList.end(), m_array.begin());
    }

    template <std::size_t N2>
    explicit constexpr MaxVector(const MaxVector<T, N2>& rhs) noexcept : m_size(rhs.size())
    {
        CORAL66_ASSERT(m_size <= N);
        std::copy(rhs.begin(), rhs.end(), m_array.begin());
    }

    constexpr MaxVector& operator=(std::initializer_list<T> initializerList) noexcept
    {
        m_size = initializerList.size();
        CORAL66_ASSERT(m_size <= N);
        std::copy(initializerList.begin(), initializerList.end(), m_array.begin());
        return *this;
    }

    template <class Iter>
    constexpr MaxVector(Iter first, Iter last) : m_size(std::distance(first, last))
    {
        CORAL66_ASSERT(m_size <= N);
        std::copy(first, last, m_array.begin());
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return m_size;
    }

    [[nodiscard]] constexpr T* begin() noexcept
    {
        return m_array.data();
    }

    [[nodiscard]] constexpr const T* begin() const noexcept
    {
        return m_array.data();
    }

    [[nodiscard]] constexpr T* end() noexcept
    {
        return m_array.data() + m_size;
    }

    [[nodiscard]] constexpr const T* end() const noexcept
    {
        return m_array.data() + m_size;
    }

    [[nodiscard]] constexpr const T* cbegin() const noexcept
    {
        return begin();
    }

    [[nodiscard]] constexpr const T* cend() const noexcept
    {
        return end();
    }

    constexpr void resize(std::size_t size) noexcept
    {
        m_size = size;
        CORAL66_ASSERT(m_size <= N);
    }

    constexpr void resize(std::size_t size, const T& value) noexcept
    {
        if (size > m_size)
        {
            std::fill(m_array.begin() + m_size, m_array.begin() + size, value);
        }
        m_size = size;
        CORAL66_ASSERT(m_size <= N);
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return m_size == 0;
    }

    [[nodiscard]] constexpr T& operator[](std::size_t n) noexcept
    {
        CORAL66_ASSERT(n < m_size);
        return m_array[n];
    }

    [[nodiscard]] constexpr const T& operator[](std::size_t n) const noexcept
    {
        CORAL66_ASSERT(n < m_size);
        return m_array[n];
    }

    [[nodiscard]] constexpr T& at(std::size_t n) noexcept
    {
        CORAL66_ASSERT(n < m_size);
        return m_array[n];
    }

    [[nodiscard]] constexpr const T& at(std::size_t n) const noexcept
    {
        CORAL66_ASSERT(n < m_size);
        return m_array[n];
    }

    [[nodiscard]] constexpr T& front() noexcept
    {
        return at(0);
    }

    [[nodiscard]] constexpr const T& front() const noexcept
    {
        return at(0);
    }

    [[nodiscard]] constexpr T& back() noexcept
    {
        CORAL66_ASSERT(!empty());
        return at(m_size - 1);
    }

    [[nodiscard]] constexpr const T& back() const noexcept
    {
        CORAL66_ASSERT(!empty());
        return at(m_size - 1);
    }

    [[nodiscard]] constexpr T* data() noexcept
    {
        return m_array.data();
    }

    [[nodiscard]] constexpr const T* data() const noexcept
    {
        return m_array.data();
    }

    constexpr void push_back(const T& value) noexcept
    {
        m_array[m_size++] = value;
        CORAL66_ASSERT(m_size <= N);
    }

    constexpr void push_back(T&& value) noexcept
    {
        m_array[m_size++] = std::move(value);
        CORAL66_ASSERT(m_size <= N);
    }

    constexpr void pop_back() noexcept
    {
        --m_size;
    }
};
} // namespace coral66
