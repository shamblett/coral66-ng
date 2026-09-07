/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <array>
#include <memory>
#include <tuple>
#include <utility>

#include "constexpr.hpp"
#include "utilities.hpp"

/**
 * AbstractIntrusiveVariant provides a facility for creating a closed set of polymorphic classes
 * (variant-like behaviour) using inheritance.
 *
 * It acts as a base class that stores a type index corresponding to the derived type, allowing
 * efficient dispatch, type checking, and casting without the overhead of virtual functions or
 * standard RTTI. It requires the list of all possible subclasses to be specified in the base
 * class template arguments.
 *
 * Key Facilities:
 * - `is<T>()`: Checks if the object is of type T.
 * - `as<T>()`: Casts the object to type T (asserts on failure).
 * - `tryAs<T>()`: Casts to type T or returns nullptr.
 * - `match(...)`: Applies a visitor to the concrete derived type (pattern matching).
 *
 * This is needed in coral66 to efficiently represent and manipulate recursive data structures like
 * AST nodes and Types in the compiler frontend.
 */

namespace coral66
{
namespace detail
{
} // namespace detail

template <class Base, class... Args>
class AbstractIntrusiveVariant
{
    suitableUInt<sizeof...(Args) - 1> m_index;

public:
    template <class T>
    explicit constexpr AbstractIntrusiveVariant(std::in_place_type_t<T>)
        : m_index(std::integral_constant<index_type, indexOf<T>()>::value)
    {
        static_assert((std::is_same_v<T, Args> || ...), "T must be one of the subclasses specified in Args...");
    }

    using index_type = decltype(m_index);
    using base_type = Base;

    template <class T>
    constexpr static index_type indexOf() noexcept
    {
        static_assert((std::is_same_v<T, Args> || ...), "Template type was not specified in the sets of possible base classes");
        constexpr std::array<bool, sizeof...(Args)> matches = {std::is_same_v<T, Args>...};
        for (std::size_t i = 0; i < matches.size(); ++i)
        {
            if (matches[i]) return i;
        }
        std::unreachable();
    }

    [[nodiscard]] constexpr index_type index() const noexcept
    {
        return m_index;
    }

    template <class T>
    [[nodiscard]] constexpr bool is() const noexcept
    {
        constexpr auto var = indexOf<T>();
        return index() == var;
    }

    template <class T, class Self>
    [[nodiscard]] constexpr auto&& as(this Self&& self) noexcept
    {
        CORAL66_ASSERT(self.index() == std::integral_constant<index_type, indexOf<T>()>::value);
        using Target = std::remove_reference_t<decltype(std::forward_like<Self>(std::declval<T&>()))>;
        return static_cast<Target&>(self);
    }

    template <class T, class Self>
    [[nodiscard]] constexpr auto tryAs(this Self&& self) noexcept
    {
        using Target = std::remove_reference_t<decltype(std::forward_like<Self>(std::declval<T&>()))>;
        if (self.index() != indexOf<T>())
        {
            return static_cast<Target*>(nullptr);
        }
        return static_cast<Target*>(&self);
    }

    template <class Self, class... F>
    decltype(auto) match(this Self&& self, F&&... f)
    {
        using Callable = decltype(detail::overload{std::forward<F>(f)...});
        using FirstArg = std::tuple_element_t<0, std::tuple<Args...>>;
        
        using ForwardedBase = decltype(std::forward_like<Self>(std::declval<AbstractIntrusiveVariant&>()));
        using ReturnType = std::invoke_result_t<Callable, decltype(std::forward_like<Self>(std::declval<FirstArg&>()))>;

        constexpr std::array<ReturnType (*)(ForwardedBase, Callable&&), sizeof...(Args)>
            calling = {{+[](ForwardedBase base, Callable&& callable) -> ReturnType
                        {
                            using TargetType = std::remove_reference_t<decltype(std::forward_like<Self>(std::declval<Args&>()))>;
                            return callable(static_cast<TargetType&>(base));
                        }...}};
        return calling[self.index()](static_cast<ForwardedBase>(self), Callable{std::forward<F>(f)...});
    }



    [[nodiscard]] friend bool operator==(const AbstractIntrusiveVariant& lhs,
                                         const AbstractIntrusiveVariant& rhs) noexcept
        requires(coral66::EqualComparable<Args, Args>&&...)
    {
        if (lhs.index() != rhs.index())
        {
            return false;
        }
        constexpr std::array<bool (*)(const AbstractIntrusiveVariant&, const AbstractIntrusiveVariant&),
                             sizeof...(Args)>
            equality = {{+[](const AbstractIntrusiveVariant& lhs1, const AbstractIntrusiveVariant& rhs1) -> bool
                         { return static_cast<const Args&>(lhs1) == static_cast<const Args&>(rhs1); }...}};
        return equality[lhs.index()](lhs, rhs);
    }

    [[nodiscard]] friend bool operator!=(const AbstractIntrusiveVariant& lhs,
                                         const AbstractIntrusiveVariant& rhs) noexcept
        requires(coral66::EqualComparable<Args, Args>&&...)
    {
        return !(lhs == rhs);
    }
};

namespace detail::AbstractIntrusiveVariant
{
template <class Base, class... Args>
// ReSharper disable once CppFunctionIsNotImplemented
auto deduceArgs(coral66::AbstractIntrusiveVariant<Base, Args...>*) -> coral66::AbstractIntrusiveVariant<Base, Args...>;
} // namespace detail::AbstractIntrusiveVariant

template <class T, class = decltype(detail::AbstractIntrusiveVariant::deduceArgs(std::declval<T*>()))>
class IntrusiveVariantDeleter
{
    static_assert(always_false<T>, "Can't delete pointer that isn't a subclass of AbstractIntrusiveVariant");
};

template <class Base, class Top, class... SubClasses>
class IntrusiveVariantDeleter<Base, AbstractIntrusiveVariant<Top, SubClasses...>>
{
public:
    IntrusiveVariantDeleter() = default;

    template <class T>
    requires(std::is_same_v<T, SubClasses> || ...) explicit IntrusiveVariantDeleter(std::default_delete<T>&&) noexcept
    {
    }

    template <class U>
    requires std::is_base_of_v<Base, U>
        // ReSharper disable once CppNonExplicitConvertingConstructor
        IntrusiveVariantDeleter(IntrusiveVariantDeleter<U>&&) noexcept
    {
    }

    void operator()(Base* pointer) const noexcept
    {
        constexpr std::array<void (*)(Base*), sizeof...(SubClasses)> deleteFuncs = {
            {+[](Base* ptr)
             {
                 if constexpr (std::is_base_of_v<Base, SubClasses>)
                 {
                     delete static_cast<SubClasses*>(ptr);
                 }
                 else
                 {
                     CORAL66_UNREACHABLE;
                 }
             }...}};
        deleteFuncs[pointer->index()](pointer);
    }
};

template <class T>
using IntrVarPtr = std::unique_ptr<T, IntrusiveVariantDeleter<T>>;

} // namespace coral66

#define CORAL66_GEN_INTR_VAR_WARNING_FUNCS(ThisClass)                                                                  \
                                                                                                                       \
    template <class T>                                                                                                 \
    requires(!std::is_same_v<T, ThisClass>)                                                                            \
        CORAL66_WARN_ON_USAGE("Function always returns false") constexpr CORAL66_ALWAYS_INLINE bool                    \
        is() const noexcept                                                                                            \
    {                                                                                                                  \
        return false;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template <class T>                                                                                                 \
    requires std::is_same_v<T, ThisClass>                                                                              \
        CORAL66_WARN_ON_USAGE("Function always returns true") constexpr CORAL66_ALWAYS_INLINE bool is() const noexcept \
    {                                                                                                                  \
        return true;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    template <class T, class Self>                                                                                     \
    requires(!std::is_same_v<T, ThisClass>) CORAL66_ERROR_ON_USAGE("Cast will never succeed")                          \
    CORAL66_ALWAYS_INLINE decltype(auto) as(this Self&& self) noexcept                                                 \
    {                                                                                                                  \
        return std::forward<Self>(self).AbstractIntrusiveVariant::template as<T>();                                                        \
    }                                                                                                                  \
                                                                                                                       \
    template <class T, class Self>                                                                                     \
    requires std::is_same_v<T, ThisClass>                                                                              \
    CORAL66_WARN_ON_USAGE("Class is already of type " #ThisClass)                                                      \
    CORAL66_ALWAYS_INLINE decltype(auto) as(this Self&& self) noexcept                                                 \
    {                                                                                                                  \
        return std::forward<Self>(self);                                                                               \
    }                                                                                                                  \
                                                                                                                       \
    template <class T, class Self>                                                                                     \
    requires(!std::is_same_v<T, ThisClass>) CORAL66_WARN_ON_USAGE("Function will always return nullptr")               \
    CORAL66_ALWAYS_INLINE auto tryAs(this Self&& self) noexcept                                                        \
    {                                                                                                                  \
        using Target = std::remove_reference_t<decltype(std::forward_like<Self>(std::declval<T&>()))>;                 \
        return static_cast<Target*>(nullptr);                                                                          \
    }                                                                                                                  \
                                                                                                                       \
    template <class T, class Self>                                                                                     \
    requires std::is_same_v<T, ThisClass> CORAL66_WARN_ON_USAGE("Class is already of type " #ThisClass)                \
    CORAL66_ALWAYS_INLINE auto tryAs(this Self&& self) noexcept                                                        \
    {                                                                                                                  \
        using Target = std::remove_reference_t<decltype(std::forward_like<Self>(std::declval<T&>()))>;                 \
        return static_cast<Target*>(&self);                                                                            \
    }                                                                                                                  \
                                                                                                                       \
    template <class Self, class... F>                                                                                  \
    CORAL66_WARN_ON_USAGE("Class type " #ThisClass " known ahead of time already")                                     \
    CORAL66_ALWAYS_INLINE decltype(auto) match(this Self&& self, F&&... f)                                             \
    {                                                                                                                  \
        return std::forward<Self>(self).AbstractIntrusiveVariant::match(std::forward<F>(f)...);                                            \
    }
