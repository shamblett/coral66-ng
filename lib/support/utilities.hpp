/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

/**
 * General support utilities.
 *
 */

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <variant>

#include "vendor/tsl/ordered_map.h"

#define CORAL66_ASSERT(...)                                                \
    do                                                                     \
    {                                                                      \
        if (!(__VA_ARGS__))                                                \
        {                                                                  \
            fprintf(stderr, __FILE__ ":%d: " #__VA_ARGS__ "\n", __LINE__); \
            std::abort();                                                  \
        }                                                                  \
    } while (0)

#define CORAL66_UNREACHABLE                                                          \
    do                                                                               \
    {                                                                                \
        std::abort(); /* So that the compiler sees code afterwards is unreachable */ \
    } while (0)

#ifdef __clang__
    #define CORAL66_NON_NULL _Nonnull
    #define CORAL66_NULLABLE _Nullable
#else
    #define CORAL66_NON_NULL
    #define CORAL66_NULLABLE
#endif

#ifndef __has_attribute
    #define __has_attribute(x) 0
#endif

#if __has_attribute(diagnose_if)
    #define CORAL66_WARN_IF(expr, message) __attribute__((diagnose_if((expr), message, "warning")))
    #define CORAL66_ERROR_IF(expr, message) __attribute__((diagnose_if((expr), message, "error")))
#else
    #define CORAL66_WARN_IF(expr, message)
    #define CORAL66_ERROR_IF(expr, message)
#endif

#define CORAL66_WARN_ON_USAGE(message) CORAL66_WARN_IF(1, message)
#define CORAL66_ERROR_ON_USAGE(message) CORAL66_ERROR_IF(1, message)

#if __has_attribute(always_inline) || __GNUC__ >= 5
    #define CORAL66_ALWAYS_INLINE inline __attribute__((always_inline))
#else
    #define CORAL66_ALWAYS_INLINE inline
#endif

namespace coral66
{
template <typename T, typename Variant>
constexpr decltype(auto) get(Variant&& variant) noexcept
{
    CORAL66_ASSERT(!variant.valueless_by_exception() && std::holds_alternative<T>(variant));
    auto* value = std::get_if<T>(&variant);
    CORAL66_ASSERT(value);
    if constexpr (std::is_lvalue_reference_v<Variant>)
    {
        return *value;
    }
    else
    {
        return std::move(*value);
    }
}

template <std::size_t i, typename Variant>
constexpr decltype(auto) get(Variant&& variant) noexcept
{
    CORAL66_ASSERT(!variant.valueless_by_exception() && variant.index() == i);
    auto* value = std::get_if<i>(&variant);
    CORAL66_ASSERT(value);
    if constexpr (std::is_lvalue_reference_v<Variant>)
    {
        return *value;
    }
    else
    {
        return std::move(*value);
    }
}

template <typename G>
struct YComb
{
    template <typename... X>
    constexpr decltype(auto) operator()(X&&... x) const
    {
        return g(*this, std::forward<X>(x)...);
    }

    template <typename... X>
    constexpr decltype(auto) operator()(X&&... x)
    {
        return g(*this, std::forward<X>(x)...);
    }

    G g;
};

template <typename G>
YComb(G) -> YComb<G>;

template <class T>
struct type_identity
{
    using type = T;
};

namespace detail
{
template <class... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overload(Ts...) -> overload<Ts...>;

template <typename G, class Ret>
struct YWithRet
{
    template <typename... X>
    Ret operator()(X&&... x) const&
    {
        return g(*this, std::forward<X>(x)...);
    }

    G g;
    type_identity<Ret> id;
};

template <typename G, class T>
YWithRet(G, type_identity<T>) -> YWithRet<G, T>;

template <std::size_t i, class Callable, class Variant>
constexpr decltype(auto) visitImpl(Callable&& callable, Variant&& variant) requires(i == 0 || i > 12)
{
    if (variant.index() == i)
    {
        return std::forward<Callable>(callable)(coral66::get<i>(std::forward<Variant>(variant)));
    }
    if constexpr (i != 0)
    {
        return visitImpl<i - 1>(std::forward<Callable>(callable), std::forward<Variant>(variant));
    }
    else
    {
        CORAL66_UNREACHABLE;
    }
}

template <std::size_t i, class Callable, class Variant>
constexpr decltype(auto) visitImpl(Callable&& callable, Variant&& variant) requires(i == 1)
{
    switch (variant.index())
    {
        case 0: return std::forward<Callable>(callable)(coral66::get<0>(std::forward<Variant>(variant)));
        case 1: return std::forward<Callable>(callable)(coral66::get<1>(std::forward<Variant>(variant)));
        default: CORAL66_UNREACHABLE;
    }
}

template <std::size_t i, class Callable, class Variant>
constexpr decltype(auto) visitImpl(Callable&& callable, Variant&& variant) requires(i == 2)
{
    switch (variant.index())
    {
        case 0: return std::forward<Callable>(callable)(coral66::get<0>(std::forward<Variant>(variant)));
        case 1: return std::forward<Callable>(callable)(coral66::get<1>(std::forward<Variant>(variant)));
        case 2: return std::forward<Callable>(callable)(coral66::get<2>(std::forward<Variant>(variant)));
        default: CORAL66_UNREACHABLE;
    }
}

template <std::size_t i, class Callable, class Variant>
constexpr decltype(auto) visitImpl(Callable&& callable, Variant&& variant) requires(i == 3)
{
    switch (variant.index())
    {
        case 0: return std::forward<Callable>(callable)(coral66::get<0>(std::forward<Variant>(variant)));
        case 1: return std::forward<Callable>(callable)(coral66::get<1>(std::forward<Variant>(variant)));
        case 2: return std::forward<Callable>(callable)(coral66::get<2>(std::forward<Variant>(variant)));
        case 3: return std::forward<Callable>(callable)(coral66::get<3>(std::forward<Variant>(variant)));
        default: CORAL66_UNREACHABLE;
    }
}

template <std::size_t i, class Callable, class Variant>
constexpr decltype(auto) visitImpl(Callable&& callable, Variant&& variant) requires(i == 4)
{
    switch (variant.index())
    {
        case 0: return std::forward<Callable>(callable)(coral66::get<0>(std::forward<Variant>(variant)));
        case 1: return std::forward<Callable>(callable)(coral66::get<1>(std::forward<Variant>(variant)));
        case 2: return std::forward<Callable>(callable)(coral66::get<2>(std::forward<Variant>(variant)));
        case 3: return std::forward<Callable>(callable)(coral66::get<3>(std::forward<Variant>(variant)));
        case 4: return std::forward<Callable>(callable)(coral66::get<4>(std::forward<Variant>(variant)));
        default: CORAL66_UNREACHABLE;
    }
}

template <std::size_t i, class Callable, class Variant>
constexpr decltype(auto) visitImpl(Callable&& callable, Variant&& variant) requires(i == 5)
{
    switch (variant.index())
    {
        case 0: return std::forward<Callable>(callable)(coral66::get<0>(std::forward<Variant>(variant)));
        case 1: return std::forward<Callable>(callable)(coral66::get<1>(std::forward<Variant>(variant)));
        case 2: return std::forward<Callable>(callable)(coral66::get<2>(std::forward<Variant>(variant)));
        case 3: return std::forward<Callable>(callable)(coral66::get<3>(std::forward<Variant>(variant)));
        case 4: return std::forward<Callable>(callable)(coral66::get<4>(std::forward<Variant>(variant)));
        case 5: return std::forward<Callable>(callable)(coral66::get<5>(std::forward<Variant>(variant)));
        default: CORAL66_UNREACHABLE;
    }
}

template <std::size_t i, class Callable, class Variant>
constexpr decltype(auto) visitImpl(Callable&& callable, Variant&& variant) requires(i == 6)
{
    switch (variant.index())
    {
        case 0: return std::forward<Callable>(callable)(coral66::get<0>(std::forward<Variant>(variant)));
        case 1: return std::forward<Callable>(callable)(coral66::get<1>(std::forward<Variant>(variant)));
        case 2: return std::forward<Callable>(callable)(coral66::get<2>(std::forward<Variant>(variant)));
        case 3: return std::forward<Callable>(callable)(coral66::get<3>(std::forward<Variant>(variant)));
        case 4: return std::forward<Callable>(callable)(coral66::get<4>(std::forward<Variant>(variant)));
        case 5: return std::forward<Callable>(callable)(coral66::get<5>(std::forward<Variant>(variant)));
        case 6: return std::forward<Callable>(callable)(coral66::get<6>(std::forward<Variant>(variant)));
        default: CORAL66_UNREACHABLE;
    }
}

template <std::size_t i, class Callable, class Variant>
constexpr decltype(auto) visitImpl(Callable&& callable, Variant&& variant) requires(i == 7)
{
    switch (variant.index())
    {
        case 0: return std::forward<Callable>(callable)(coral66::get<0>(std::forward<Variant>(variant)));
        case 1: return std::forward<Callable>(callable)(coral66::get<1>(std::forward<Variant>(variant)));
        case 2: return std::forward<Callable>(callable)(coral66::get<2>(std::forward<Variant>(variant)));
        case 3: return std::forward<Callable>(callable)(coral66::get<3>(std::forward<Variant>(variant)));
        case 4: return std::forward<Callable>(callable)(coral66::get<4>(std::forward<Variant>(variant)));
        case 5: return std::forward<Callable>(callable)(coral66::get<5>(std::forward<Variant>(variant)));
        case 6: return std::forward<Callable>(callable)(coral66::get<6>(std::forward<Variant>(variant)));
        case 7: return std::forward<Callable>(callable)(coral66::get<7>(std::forward<Variant>(variant)));
        default: CORAL66_UNREACHABLE;
    }
}

template <std::size_t i, class Callable, class Variant>
constexpr decltype(auto) visitImpl(Callable&& callable, Variant&& variant) requires(i == 8)
{
    switch (variant.index())
    {
        case 0: return std::forward<Callable>(callable)(coral66::get<0>(std::forward<Variant>(variant)));
        case 1: return std::forward<Callable>(callable)(coral66::get<1>(std::forward<Variant>(variant)));
        case 2: return std::forward<Callable>(callable)(coral66::get<2>(std::forward<Variant>(variant)));
        case 3: return std::forward<Callable>(callable)(coral66::get<3>(std::forward<Variant>(variant)));
        case 4: return std::forward<Callable>(callable)(coral66::get<4>(std::forward<Variant>(variant)));
        case 5: return std::forward<Callable>(callable)(coral66::get<5>(std::forward<Variant>(variant)));
        case 6: return std::forward<Callable>(callable)(coral66::get<6>(std::forward<Variant>(variant)));
        case 7: return std::forward<Callable>(callable)(coral66::get<7>(std::forward<Variant>(variant)));
        case 8: return std::forward<Callable>(callable)(coral66::get<8>(std::forward<Variant>(variant)));
        default: CORAL66_UNREACHABLE;
    }
}

template <std::size_t i, class Callable, class Variant>
constexpr decltype(auto) visitImpl(Callable&& callable, Variant&& variant) requires(i == 9)
{
    switch (variant.index())
    {
        case 0: return std::forward<Callable>(callable)(coral66::get<0>(std::forward<Variant>(variant)));
        case 1: return std::forward<Callable>(callable)(coral66::get<1>(std::forward<Variant>(variant)));
        case 2: return std::forward<Callable>(callable)(coral66::get<2>(std::forward<Variant>(variant)));
        case 3: return std::forward<Callable>(callable)(coral66::get<3>(std::forward<Variant>(variant)));
        case 4: return std::forward<Callable>(callable)(coral66::get<4>(std::forward<Variant>(variant)));
        case 5: return std::forward<Callable>(callable)(coral66::get<5>(std::forward<Variant>(variant)));
        case 6: return std::forward<Callable>(callable)(coral66::get<6>(std::forward<Variant>(variant)));
        case 7: return std::forward<Callable>(callable)(coral66::get<7>(std::forward<Variant>(variant)));
        case 8: return std::forward<Callable>(callable)(coral66::get<8>(std::forward<Variant>(variant)));
        case 9: return std::forward<Callable>(callable)(coral66::get<9>(std::forward<Variant>(variant)));
        default: CORAL66_UNREACHABLE;
    }
}

template <std::size_t i, class Callable, class Variant>
constexpr decltype(auto) visitImpl(Callable&& callable, Variant&& variant) requires(i == 10)
{
    switch (variant.index())
    {
        case 0: return std::forward<Callable>(callable)(coral66::get<0>(std::forward<Variant>(variant)));
        case 1: return std::forward<Callable>(callable)(coral66::get<1>(std::forward<Variant>(variant)));
        case 2: return std::forward<Callable>(callable)(coral66::get<2>(std::forward<Variant>(variant)));
        case 3: return std::forward<Callable>(callable)(coral66::get<3>(std::forward<Variant>(variant)));
        case 4: return std::forward<Callable>(callable)(coral66::get<4>(std::forward<Variant>(variant)));
        case 5: return std::forward<Callable>(callable)(coral66::get<5>(std::forward<Variant>(variant)));
        case 6: return std::forward<Callable>(callable)(coral66::get<6>(std::forward<Variant>(variant)));
        case 7: return std::forward<Callable>(callable)(coral66::get<7>(std::forward<Variant>(variant)));
        case 8: return std::forward<Callable>(callable)(coral66::get<8>(std::forward<Variant>(variant)));
        case 9: return std::forward<Callable>(callable)(coral66::get<9>(std::forward<Variant>(variant)));
        case 10: return std::forward<Callable>(callable)(coral66::get<10>(std::forward<Variant>(variant)));
        default: CORAL66_UNREACHABLE;
    }
}

template <std::size_t i, class Callable, class Variant>
constexpr decltype(auto) visitImpl(Callable&& callable, Variant&& variant) requires(i == 11)
{
    switch (variant.index())
    {
        case 0: return std::forward<Callable>(callable)(coral66::get<0>(std::forward<Variant>(variant)));
        case 1: return std::forward<Callable>(callable)(coral66::get<1>(std::forward<Variant>(variant)));
        case 2: return std::forward<Callable>(callable)(coral66::get<2>(std::forward<Variant>(variant)));
        case 3: return std::forward<Callable>(callable)(coral66::get<3>(std::forward<Variant>(variant)));
        case 4: return std::forward<Callable>(callable)(coral66::get<4>(std::forward<Variant>(variant)));
        case 5: return std::forward<Callable>(callable)(coral66::get<5>(std::forward<Variant>(variant)));
        case 6: return std::forward<Callable>(callable)(coral66::get<6>(std::forward<Variant>(variant)));
        case 7: return std::forward<Callable>(callable)(coral66::get<7>(std::forward<Variant>(variant)));
        case 8: return std::forward<Callable>(callable)(coral66::get<8>(std::forward<Variant>(variant)));
        case 9: return std::forward<Callable>(callable)(coral66::get<9>(std::forward<Variant>(variant)));
        case 10: return std::forward<Callable>(callable)(coral66::get<10>(std::forward<Variant>(variant)));
        case 11: return std::forward<Callable>(callable)(coral66::get<11>(std::forward<Variant>(variant)));
        default: CORAL66_UNREACHABLE;
    }
}

template <std::size_t i, class Callable, class Variant>
constexpr decltype(auto) visitImpl(Callable&& callable, Variant&& variant) requires(i == 12)
{
    switch (variant.index())
    {
        case 0: return std::forward<Callable>(callable)(coral66::get<0>(std::forward<Variant>(variant)));
        case 1: return std::forward<Callable>(callable)(coral66::get<1>(std::forward<Variant>(variant)));
        case 2: return std::forward<Callable>(callable)(coral66::get<2>(std::forward<Variant>(variant)));
        case 3: return std::forward<Callable>(callable)(coral66::get<3>(std::forward<Variant>(variant)));
        case 4: return std::forward<Callable>(callable)(coral66::get<4>(std::forward<Variant>(variant)));
        case 5: return std::forward<Callable>(callable)(coral66::get<5>(std::forward<Variant>(variant)));
        case 6: return std::forward<Callable>(callable)(coral66::get<6>(std::forward<Variant>(variant)));
        case 7: return std::forward<Callable>(callable)(coral66::get<7>(std::forward<Variant>(variant)));
        case 8: return std::forward<Callable>(callable)(coral66::get<8>(std::forward<Variant>(variant)));
        case 9: return std::forward<Callable>(callable)(coral66::get<9>(std::forward<Variant>(variant)));
        case 10: return std::forward<Callable>(callable)(coral66::get<10>(std::forward<Variant>(variant)));
        case 11: return std::forward<Callable>(callable)(coral66::get<11>(std::forward<Variant>(variant)));
        case 12: return std::forward<Callable>(callable)(coral66::get<12>(std::forward<Variant>(variant)));
        default: CORAL66_UNREACHABLE;
    }
}

template <class Callable, class Variant>
constexpr decltype(auto) visit(Callable&& callable, Variant&& variant)
{
    CORAL66_ASSERT(!variant.valueless_by_exception());
    return visitImpl<std::variant_size_v<std::decay_t<Variant>> - 1>(std::forward<Callable>(callable),
                                                                     std::forward<Variant>(variant));
}
} // namespace detail

template <typename Variant, typename... Matchers>
constexpr decltype(auto) match(const Variant&& variant, const Matchers&&... matchers)
{
    CORAL66_ASSERT(!variant.valueless_by_exception());
    return detail::visit(detail::overload{std::forward<Matchers>(matchers)...}, std::forward<Variant>(variant));
}

template <typename Variant, typename... Matchers>
constexpr decltype(auto) match(Variant&& variant, Matchers&&... matchers)
{
    CORAL66_ASSERT(!variant.valueless_by_exception());
    return detail::visit(detail::overload{std::forward<Matchers>(matchers)...}, std::forward<Variant>(variant));
}

template <class Ret, typename Variant, typename... Matchers>
constexpr void match(Variant&& variant, Matchers&&... matchers)
{
    static_assert(std::is_void_v<Ret>, "Only explicit return type of void allowed");
    detail::visit(detail::overload{std::forward<Matchers>(matchers)...}, std::forward<Variant>(variant));
}

template <typename Variant, typename... Matchers>
constexpr decltype(auto) matchWithSelf(Variant&& variant, Matchers&&... matchers)
{
    return detail::visit(YComb{detail::overload{std::forward<Matchers>(matchers)...}}, std::forward<Variant>(variant));
}

template <class Ret, typename Variant, typename... Matchers>
constexpr void matchWithSelf(Variant&& variant, Matchers&&... matchers)
{
    static_assert(std::is_void_v<Ret>, "Only explicit return type of void allowed");
    detail::visit(detail::YWithRet{detail::overload{std::forward<Matchers>(matchers)...}, type_identity<Ret>{}},
                  std::forward<Variant>(variant));
}

template <typename T, typename... Ts>
constexpr size_t getIndex(const std::variant<Ts...>&) noexcept
{
    size_t r = 0;
    auto test = [&](bool b)
    {
        if (!b)
            ++r;
        return b;
    };
    (test(std::is_same_v<T, Ts>) || ...);
    return r;
}

template <template <class> class Hasher = std::hash, class... Args>
constexpr std::size_t hashCombine(const Args&... args)
{
    std::size_t seed = 0;
    ((seed ^= Hasher<std::decay_t<Args>>{}(args) + 0x9e3779b9 + (seed << 6) + (seed >> 2)), ...);
    return seed;
}

template <template <class> class Hasher = std::hash, class... Args>
constexpr std::size_t rawHashCombine(Args... args)
{
    static_assert((std::is_integral_v<Args> && ...));
    std::size_t seed = 0;
    ((seed ^= args + 0x9e3779b9 + (seed << 6) + (seed >> 2)), ...);
    return seed;
}

template <class T1, class T2>
constexpr T1 roundUpTo(T1 number, T2 multiple)
{
    static_assert(std::is_integral_v<T1> && std::is_integral_v<T2>);
    if (multiple == 0)
    {
        return number;
    }

    auto remainder = number % multiple;
    if (remainder == 0)
    {
        return number;
    }

    return number + multiple - remainder;
}

namespace detail
{
template <class F, class... Args>
class BindFrontImpl
{
    F f;
    std::tuple<Args...> front;

public:
    explicit BindFrontImpl(F f, std::tuple<Args...> args) : f(std::move(f)), front(std::move(args)) {}

    template <class... Last>
    decltype(auto) operator()(Last&&... last) & noexcept(std::is_nothrow_invocable_v<F, Args..., Last...>)
    {
        return std::apply([&]<typename... T0>(T0&&... members) -> decltype(auto)
                          { return std::invoke(f, std::forward<T0>(members)..., std::forward<Last>(last)...); }, front);
    }

    template <class... Last>
    decltype(auto) operator()(Last&&... last) const& noexcept(std::is_nothrow_invocable_v<F, Args..., Last...>)
    {
        return std::apply([&]<typename... T0>(T0&&... members) -> decltype(auto)
                          { return std::invoke(f, std::forward<T0>(members)..., std::forward<Last>(last)...); }, front);
    }

    template <class... Last>
    decltype(auto) operator()(Last&&... last) && noexcept(std::is_nothrow_invocable_v<F, Args..., Last...>)
    {
        return std::apply(
            [&]<typename... T0>(T0&&... members) -> decltype(auto)
            { return std::invoke(std::move(f), std::forward<T0>(members)..., std::forward<Last>(last)...); },
            std::move(front));
    }

    template <class... Last>
    decltype(auto) operator()(Last&&... last) const&& noexcept(std::is_nothrow_invocable_v<F, Args..., Last...>)
    {
        return std::apply(
            [&]<typename... T0>(T0&&... members) -> decltype(auto)
            { return std::invoke(std::move(f), std::forward<T0>(members)..., std::forward<Last>(last)...); },
            std::move(front));
    }
};

} // namespace detail

template <class F, class... Args>
auto bind_front(F&& f, Args&&... args)
{
    return detail::BindFrontImpl(std::forward<F>(f), std::make_tuple(std::forward<Args>(args)...));
}

template <class F, class G>
constexpr auto compose(F&& f, G&& g)
{
    return [f = std::forward<F>(f), g = std::forward<G>(g)]<typename T0>(T0&& value)
    {
        static_assert(std::is_invocable_v<std::decay_t<G>, std::decay_t<T0>>);
        static_assert(std::is_invocable_v<std::decay_t<F>, decltype(std::invoke(std::declval<std::decay_t<G>>(),
                                                                                std::forward<T0>(value)))>);
        return std::invoke(f, std::invoke(g, std::forward<T0>(value)));
    };
}

template <class T>
class not_null
{
    T* CORAL66_NON_NULL m_ptr;

public:
    explicit constexpr not_null(T* CORAL66_NON_NULL ptr) : m_ptr(ptr)
    {
        CORAL66_ASSERT(m_ptr);
    }

    template <class U>
    requires std::is_convertible_v<U, T*>
        // ReSharper disable once CppNonExplicitConvertingConstructor
        constexpr not_null(U&& u) : m_ptr(std::forward<U>(u))
    {
        CORAL66_ASSERT(m_ptr);
    }

    template <class U>
    requires std::is_convertible_v<U*, T*> explicit constexpr not_null(const not_null<U>& rhs) : not_null(rhs.get())
    {
    }

    explicit not_null(std::nullptr_t) = delete;
    not_null& operator=(std::nullptr_t) = delete;

    constexpr T* CORAL66_NON_NULL get() const
    {
        CORAL66_ASSERT(m_ptr);
        return m_ptr;
    }

    explicit constexpr operator T* CORAL66_NON_NULL() const
    {
        return get();
    }

    constexpr T* CORAL66_NON_NULL operator->() const
    {
        return get();
    }

    constexpr T& operator*() const
    {
        return *get();
    }

    not_null& operator++() = delete;
    not_null& operator--() = delete;
    not_null& operator++(int) = delete;
    not_null& operator--(int) = delete;
    not_null& operator+=(std::ptrdiff_t) = delete;
    not_null& operator-=(std::ptrdiff_t) = delete;
    void operator[](std::ptrdiff_t) const = delete;
};

template <class T, class U>
bool operator==(not_null<T> lhs, not_null<U> rhs)
{
    return lhs.get() == rhs.get();
}

template <class T, class U>
bool operator!=(not_null<T> lhs, not_null<U> rhs)
{
    return !(rhs == lhs);
}

template <class T, class U>
bool operator<(not_null<T> lhs, not_null<U> rhs)
{
    return lhs.get() < rhs.get();
}

template <class T, class U>
bool operator>(not_null<T> lhs, not_null<U> rhs)
{
    return rhs < lhs;
}

template <class T, class U>
bool operator<=(not_null<T> lhs, not_null<U> rhs)
{
    return !(rhs < lhs);
}

template <class T, class U>
bool operator>=(not_null<T> lhs, not_null<U> rhs)
{
    return !(lhs < rhs);
}

template <class T, class U>
std::ptrdiff_t operator-(not_null<T>, not_null<U>) = delete;
template <class T>
not_null<T> operator-(not_null<T>, std::ptrdiff_t) = delete;
template <class T>
not_null<T> operator+(not_null<T>, std::ptrdiff_t) = delete;
template <class T>
not_null<T> operator+(std::ptrdiff_t, not_null<T>) = delete;

template <class Key, class Mapped, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<Key, Mapped>>,
          class ValueTypeContainer = std::vector<std::pair<Key, Mapped>, Allocator>>
using ordered_map = ::tsl::ordered_map<Key, Mapped, Hash, KeyEqual, Allocator, ValueTypeContainer>;

} // namespace coral66

template <class T>
struct std::hash<coral66::not_null<T>> // NOLINT(*-dcl58-cpp)
{
    std::size_t operator()(coral66::not_null<T> value) const
    {
        return hash<T*>{}(value.get());
    }
}; // namespace std
