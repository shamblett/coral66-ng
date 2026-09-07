/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>

#include "abstract_intrusive_variant.hpp"
#include "constexpr.hpp"

/**
 * IntrusiveVariantValue provides value semantics for AbstractIntrusiveVariant hierarchies, similar to std::variant.
 *
 * Unlike `AbstractIntrusiveVariant` which is designed for pointer/reference-based polymorphism,
 * `IntrusiveVariantValue` stores the object instance directly within its own memory layout (using a union-like
 * storage buffer large enough for the biggest subclass).
 *
 * Key Features:
 * - Inline storage: Avoids heap allocation for the variant object itself.
 * - Value semantics: Supports copy/move construction and assignment, properly handling the active subclass.
 * - Type Safety: Ensures the active type is tracked and managed correctly via the base `AbstractIntrusiveVariant`
 * mechanisms.
 * - interoperability: Can be compared and accessed like the underlying base class type.
 */
namespace coral66
{
namespace detail::IntrusiveVariantStorage
{
template <class... Args>
// ReSharper disable once CppFunctionIsNotImplemented
auto deduceArgs(coral66::AbstractIntrusiveVariant<Args...>*) -> coral66::AbstractIntrusiveVariant<Args...>;

template <class Base, class... SubClasses>
class IntrusiveVariantStorageBase
{
protected:
    alignas(SubClasses...) std::byte m_storage[std::max({sizeof(SubClasses)...})] = {};
    suitableUInt<sizeof...(SubClasses)> m_index;

    template <class Self>
    [[nodiscard]] decltype(auto) get(this Self&& self)
    {
        using BaseType = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const Base, Base>;
        constexpr std::array<BaseType& (*)(std::byte*), sizeof...(SubClasses)> converters = {
            +[](std::byte* ptr) -> BaseType&
            {
                using SubType =
                    std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const SubClasses, SubClasses>;
                return *std::launder(reinterpret_cast<SubType*>(ptr));
            }...};
        return converters[self.m_index](const_cast<std::byte*>(self.m_storage));
    }
};

enum class SpecialMem
{
    Trivial,
    Exists,
    Deleted
};

template <class Base, SpecialMem status, class... SubClasses>
struct IntrusiveVariantStorageDestruct;

template <class Base, class... SubClasses>
struct IntrusiveVariantStorageDestruct<Base, SpecialMem::Trivial, SubClasses...>
    : IntrusiveVariantStorageBase<Base, SubClasses...>
{
protected:
    // ReSharper disable once CppMemberFunctionMayBeStatic
    void destruct() {}

public:
    IntrusiveVariantStorageDestruct() noexcept = default;

    ~IntrusiveVariantStorageDestruct() noexcept = default;

    IntrusiveVariantStorageDestruct(const IntrusiveVariantStorageDestruct&) noexcept = default;
    IntrusiveVariantStorageDestruct& operator=(const IntrusiveVariantStorageDestruct&) noexcept = default;
    IntrusiveVariantStorageDestruct(IntrusiveVariantStorageDestruct&&) noexcept = default;
    IntrusiveVariantStorageDestruct& operator=(IntrusiveVariantStorageDestruct&&) noexcept = default;
};

template <class Base, class... SubClasses>
struct IntrusiveVariantStorageDestruct<Base, SpecialMem::Exists, SubClasses...>
    : IntrusiveVariantStorageBase<Base, SubClasses...>
{
protected:
    void destruct()
    {
        constexpr std::array<void (*)(Base*), sizeof...(SubClasses)> destructorFuncs = {
            {+[](Base* ptr) { std::destroy_at(static_cast<SubClasses*>(ptr)); }...}};
        destructorFuncs[this->m_index](&this->get());
    }

public:
    IntrusiveVariantStorageDestruct() = default;

    ~IntrusiveVariantStorageDestruct()
    {
        destruct();
    }

    IntrusiveVariantStorageDestruct(const IntrusiveVariantStorageDestruct&) noexcept = default;
    IntrusiveVariantStorageDestruct& operator=(const IntrusiveVariantStorageDestruct&) noexcept = default;
    IntrusiveVariantStorageDestruct(IntrusiveVariantStorageDestruct&&) noexcept = default;
    IntrusiveVariantStorageDestruct& operator=(IntrusiveVariantStorageDestruct&&) noexcept = default;
};

template <class T>
constexpr SpecialMem dtor()
{
    return std::is_trivially_destructible_v<T> ? SpecialMem::Trivial : SpecialMem::Exists;
}

template <class Base, SpecialMem status, class... SubClasses>
struct IntrusiveVariantStorageCopy;

template <class Base, class... SubClasses>
struct IntrusiveVariantStorageCopy<Base, SpecialMem::Trivial, SubClasses...>
    : IntrusiveVariantStorageDestruct<Base, std::max({dtor<SubClasses>()...}), SubClasses...>
{
protected:
    void copyConstruct(const IntrusiveVariantStorageCopy& rhs)
    {
        constexpr std::array<void (*)(std::byte*, Base*), sizeof...(SubClasses)> copyFuncs = {
            {+[](std::byte* storage, Base* ptr)
             { std::construct_at(reinterpret_cast<SubClasses*>(storage), *static_cast<SubClasses*>(ptr)); }...}};
        copyFuncs[rhs.m_index](this->m_storage, &rhs.get());
        this->m_storage = rhs.m_index;
    }

public:
    IntrusiveVariantStorageCopy() noexcept = default;

    IntrusiveVariantStorageCopy(const IntrusiveVariantStorageCopy&) noexcept = default;

    IntrusiveVariantStorageCopy& operator=(const IntrusiveVariantStorageCopy&) noexcept = default;
    IntrusiveVariantStorageCopy(IntrusiveVariantStorageCopy&&) noexcept = default;
    IntrusiveVariantStorageCopy& operator=(IntrusiveVariantStorageCopy&&) noexcept = default;
};

template <class Base, class... SubClasses>
struct IntrusiveVariantStorageCopy<Base, SpecialMem::Deleted, SubClasses...>
    : IntrusiveVariantStorageDestruct<Base, std::max({dtor<SubClasses>()...}), SubClasses...>
{
protected:
    void copyConstruct(const IntrusiveVariantStorageCopy&) {}

public:
    IntrusiveVariantStorageCopy() noexcept = default;

    IntrusiveVariantStorageCopy(const IntrusiveVariantStorageCopy&) = delete;

    IntrusiveVariantStorageCopy& operator=(const IntrusiveVariantStorageCopy&) noexcept = default;
    IntrusiveVariantStorageCopy(IntrusiveVariantStorageCopy&&) noexcept = default;
    IntrusiveVariantStorageCopy& operator=(IntrusiveVariantStorageCopy&&) noexcept = default;
};

template <class Base, class... SubClasses>
struct IntrusiveVariantStorageCopy<Base, SpecialMem::Exists, SubClasses...>
    : IntrusiveVariantStorageDestruct<Base, std::max({dtor<SubClasses>()...}), SubClasses...>
{
protected:
    void copyConstruct(const IntrusiveVariantStorageCopy& rhs)
    {
        constexpr std::array<void (*)(std::byte*, const Base*), sizeof...(SubClasses)> copyFuncs = {
            {+[](std::byte* storage, const Base* ptr)
             { std::construct_at(reinterpret_cast<SubClasses*>(storage), *static_cast<const SubClasses*>(ptr)); }...}};
        copyFuncs[rhs.m_index](this->m_storage, &rhs.get());
        this->m_index = rhs.m_index;
    }

public:
    IntrusiveVariantStorageCopy() = default;

    IntrusiveVariantStorageCopy(const IntrusiveVariantStorageCopy& rhs) noexcept(
        (std::is_nothrow_copy_constructible_v<SubClasses> && ...))
    {
        copyConstruct(rhs);
    }

    IntrusiveVariantStorageCopy& operator=(const IntrusiveVariantStorageCopy&) noexcept = default;

    IntrusiveVariantStorageCopy(IntrusiveVariantStorageCopy&&) noexcept = default;
    IntrusiveVariantStorageCopy& operator=(IntrusiveVariantStorageCopy&&) noexcept = default;
};

template <class T>
constexpr SpecialMem copyCtor()
{
    if constexpr (!std::is_copy_constructible_v<T>)
    {
        return SpecialMem::Deleted;
    }
    else if constexpr (std::is_trivially_copy_constructible_v<T>)
    {
        return SpecialMem::Trivial;
    }
    return SpecialMem::Exists;
}

template <class Base, SpecialMem status, class... SubClasses>
struct IntrusiveVariantStorageCopyAss;

template <class Base, class... SubClasses>
struct IntrusiveVariantStorageCopyAss<Base, SpecialMem::Deleted, SubClasses...>
    : IntrusiveVariantStorageCopy<Base, std::max({copyCtor<SubClasses>()...}), SubClasses...>
{
    IntrusiveVariantStorageCopyAss() noexcept = default;

    IntrusiveVariantStorageCopyAss& operator=(const IntrusiveVariantStorageCopyAss&) = delete;

    ~IntrusiveVariantStorageCopyAss() = default;
    IntrusiveVariantStorageCopyAss(const IntrusiveVariantStorageCopyAss&) noexcept = default;
    IntrusiveVariantStorageCopyAss(IntrusiveVariantStorageCopyAss&&) noexcept = default;
    IntrusiveVariantStorageCopyAss& operator=(IntrusiveVariantStorageCopyAss&&) noexcept = default;
};

template <class Base, class... SubClasses>
struct IntrusiveVariantStorageCopyAss<Base, SpecialMem::Trivial, SubClasses...>
    : IntrusiveVariantStorageCopy<Base, std::max({copyCtor<SubClasses>()...}), SubClasses...>
{
    IntrusiveVariantStorageCopyAss() noexcept = default;

    IntrusiveVariantStorageCopyAss& operator=(const IntrusiveVariantStorageCopyAss&) noexcept = default;

    ~IntrusiveVariantStorageCopyAss() = default;
    IntrusiveVariantStorageCopyAss(const IntrusiveVariantStorageCopyAss&) noexcept = default;
    IntrusiveVariantStorageCopyAss(IntrusiveVariantStorageCopyAss&&) noexcept = default;
    IntrusiveVariantStorageCopyAss& operator=(IntrusiveVariantStorageCopyAss&&) noexcept = default;
};

template <class Base, class... SubClasses>
struct IntrusiveVariantStorageCopyAss<Base, SpecialMem::Exists, SubClasses...>
    : IntrusiveVariantStorageCopy<Base, std::max({copyCtor<SubClasses>()...}), SubClasses...>
{
    IntrusiveVariantStorageCopyAss() noexcept = default;

    IntrusiveVariantStorageCopyAss& operator=(const IntrusiveVariantStorageCopyAss& rhs) noexcept(
        (std::is_nothrow_copy_constructible_v<SubClasses> && ...)
        && (std::is_nothrow_copy_assignable_v<SubClasses> && ...))
    {
        if (this->m_index == rhs.m_index)
        {
            constexpr std::array<void (*)(Base*, const Base*), sizeof...(SubClasses)> copyFuncs = {
                {+[](Base* storage, const Base* ptr)
                 { *static_cast<SubClasses*>(storage) = *static_cast<const SubClasses*>(ptr); }...}};
            copyFuncs[rhs.m_index](&this->get(), &rhs.get());
        }
        else
        {
            this->destruct();
            this->copyConstruct(rhs);
        }
        return *this;
    }

    ~IntrusiveVariantStorageCopyAss() noexcept = default;
    IntrusiveVariantStorageCopyAss(const IntrusiveVariantStorageCopyAss&) noexcept = default;
    IntrusiveVariantStorageCopyAss(IntrusiveVariantStorageCopyAss&&) noexcept = default;
    IntrusiveVariantStorageCopyAss& operator=(IntrusiveVariantStorageCopyAss&&) noexcept = default;
};

template <class T>
constexpr SpecialMem copyAss()
{
    if constexpr (std::is_trivially_copy_assignable_v<T> && std::is_trivially_destructible_v<T>)
    {
        return SpecialMem::Trivial;
    }
    else if constexpr (!std::is_copy_assignable_v<T> || !std::is_copy_constructible_v<T>)
    {
        return SpecialMem::Deleted;
    }
    return SpecialMem::Exists;
}

template <class Base, SpecialMem status, class... SubClasses>
struct IntrusiveVariantStorageMove;

template <class Base, class... SubClasses>
struct IntrusiveVariantStorageMove<Base, SpecialMem::Trivial, SubClasses...>
    : IntrusiveVariantStorageCopyAss<Base, std::max({copyAss<SubClasses>()...}), SubClasses...>
{
protected:
    void moveConstruct(IntrusiveVariantStorageMove&& rhs)
    {
        constexpr std::array<void (*)(std::byte*, Base*), sizeof...(SubClasses)> moveFuncs = {
            {+[](std::byte* storage, Base* ptr)
             {
                 std::construct_at(reinterpret_cast<SubClasses*>(storage), std::move(*static_cast<SubClasses*>(ptr)));
             }...}};
        moveFuncs[rhs.m_index](this->m_storage, &rhs.get());
        this->m_index = rhs.m_index;
    }

public:
    IntrusiveVariantStorageMove() noexcept = default;

    IntrusiveVariantStorageMove(const IntrusiveVariantStorageMove&) noexcept = default;
    IntrusiveVariantStorageMove& operator=(const IntrusiveVariantStorageMove&) noexcept = default;
    IntrusiveVariantStorageMove(IntrusiveVariantStorageMove&&) noexcept = default;
    IntrusiveVariantStorageMove& operator=(IntrusiveVariantStorageMove&&) noexcept = default;
};

template <class Base, class... SubClasses>
struct IntrusiveVariantStorageMove<Base, SpecialMem::Deleted, SubClasses...>
    : IntrusiveVariantStorageCopyAss<Base, std::max({copyAss<SubClasses>()...}), SubClasses...>
{
protected:
    void moveConstruct(IntrusiveVariantStorageMove&&) {}

public:
    IntrusiveVariantStorageMove() noexcept = default;

    IntrusiveVariantStorageMove(IntrusiveVariantStorageMove&&) = delete;

    IntrusiveVariantStorageMove& operator=(const IntrusiveVariantStorageMove&) noexcept = default;
    IntrusiveVariantStorageMove(const IntrusiveVariantStorageMove&) noexcept = default;
    IntrusiveVariantStorageMove& operator=(IntrusiveVariantStorageMove&&) noexcept = default;
};

template <class Base, class... SubClasses>
struct IntrusiveVariantStorageMove<Base, SpecialMem::Exists, SubClasses...>
    : IntrusiveVariantStorageCopyAss<Base, std::max({copyAss<SubClasses>()...}), SubClasses...>
{
protected:
    void moveConstruct(IntrusiveVariantStorageMove&& rhs)
    {
        constexpr std::array<void (*)(std::byte*, Base*), sizeof...(SubClasses)> moveFuncs = {
            {+[](std::byte* storage, Base* ptr)
             {
                 std::construct_at(reinterpret_cast<SubClasses*>(storage), std::move(*static_cast<SubClasses*>(ptr)));
             }...}};
        moveFuncs[rhs.m_index](this->m_storage, &rhs.get());
        this->m_index = rhs.m_index;
    }

public:
    IntrusiveVariantStorageMove() = default;

    IntrusiveVariantStorageMove(IntrusiveVariantStorageMove&& rhs) noexcept(
        (std::is_nothrow_move_constructible_v<SubClasses> && ...))
    {
        moveConstruct(std::move(rhs));
    }

    IntrusiveVariantStorageMove(const IntrusiveVariantStorageMove&) noexcept = default;
    IntrusiveVariantStorageMove& operator=(const IntrusiveVariantStorageMove&) noexcept = default;
    IntrusiveVariantStorageMove& operator=(IntrusiveVariantStorageMove&&) noexcept = default;
};

template <class T>
constexpr SpecialMem moveCtor()
{
    if constexpr (!std::is_move_constructible_v<T>)
    {
        return SpecialMem::Deleted;
    }
    else if constexpr (std::is_trivially_move_constructible_v<T>)
    {
        return SpecialMem::Trivial;
    }
    return SpecialMem::Exists;
}

template <class Base, SpecialMem status, class... SubClasses>
struct IntrusiveVariantStorageMoveAss;

template <class Base, class... SubClasses>
struct IntrusiveVariantStorageMoveAss<Base, SpecialMem::Deleted, SubClasses...>
    : IntrusiveVariantStorageMove<Base, std::max({moveCtor<SubClasses>()...}), SubClasses...>
{
    IntrusiveVariantStorageMoveAss() noexcept = default;

    ~IntrusiveVariantStorageMoveAss() = default;

    IntrusiveVariantStorageMoveAss& operator=(IntrusiveVariantStorageMoveAss&&) = delete;

    IntrusiveVariantStorageMoveAss(const IntrusiveVariantStorageMoveAss&) noexcept = default;
    IntrusiveVariantStorageMoveAss& operator=(const IntrusiveVariantStorageMoveAss&) noexcept = default;
    IntrusiveVariantStorageMoveAss(IntrusiveVariantStorageMoveAss&&) noexcept = default;
};

template <class Base, class... SubClasses>
struct IntrusiveVariantStorageMoveAss<Base, SpecialMem::Trivial, SubClasses...>
    : IntrusiveVariantStorageMove<Base, std::max({moveCtor<SubClasses>()...}), SubClasses...>
{
    IntrusiveVariantStorageMoveAss() noexcept = default;

    IntrusiveVariantStorageMoveAss& operator=(IntrusiveVariantStorageMoveAss&&) noexcept = default;

    ~IntrusiveVariantStorageMoveAss() = default;
    IntrusiveVariantStorageMoveAss(const IntrusiveVariantStorageMoveAss&) noexcept = default;
    IntrusiveVariantStorageMoveAss& operator=(const IntrusiveVariantStorageMoveAss&) noexcept = default;
    IntrusiveVariantStorageMoveAss(IntrusiveVariantStorageMoveAss&&) noexcept = default;
};

template <class Base, class... SubClasses>
struct IntrusiveVariantStorageMoveAss<Base, SpecialMem::Exists, SubClasses...>
    : IntrusiveVariantStorageMove<Base, std::max({moveCtor<SubClasses>()...}), SubClasses...>
{
    IntrusiveVariantStorageMoveAss() noexcept = default;

    IntrusiveVariantStorageMoveAss& operator=(IntrusiveVariantStorageMoveAss&& rhs) noexcept(
        (std::is_nothrow_move_constructible_v<SubClasses> && ...)
        && (std::is_nothrow_move_assignable_v<SubClasses> && ...))
    {
        if (this->m_index == rhs.m_index)
        {
            constexpr std::array<void (*)(Base*, Base*), sizeof...(SubClasses)> moveFuncs = {
                {+[](Base* storage, Base* ptr)
                 { *static_cast<SubClasses*>(storage) = std::move(*static_cast<SubClasses*>(ptr)); }...}};
            moveFuncs[rhs.m_index](&this->get(), &rhs.get());
        }
        else
        {
            this->destruct();
            this->moveConstruct(std::move(rhs));
        }
        return *this;
    }

    ~IntrusiveVariantStorageMoveAss() noexcept = default;
    IntrusiveVariantStorageMoveAss(const IntrusiveVariantStorageMoveAss&) noexcept = default;
    IntrusiveVariantStorageMoveAss(IntrusiveVariantStorageMoveAss&&) noexcept = default;
    IntrusiveVariantStorageMoveAss& operator=(const IntrusiveVariantStorageMoveAss&) noexcept = default;
};

template <class T>
constexpr SpecialMem moveAss()
{
    if constexpr (std::is_trivially_move_assignable_v<T> && std::is_trivially_destructible_v<T>)
    {
        return SpecialMem::Trivial;
    }
    else if constexpr (!std::is_move_assignable_v<T> || !std::is_move_constructible_v<T>)
    {
        return SpecialMem::Deleted;
    }
    return SpecialMem::Exists;
}

} // namespace detail::IntrusiveVariantStorage

template <class T, class = decltype(detail::IntrusiveVariantStorage::deduceArgs(std::declval<T*>()))>
class IntrusiveVariantValue
{
    static_assert(always_false<T>,
                  "InstrusiveVariantStorage can only store a class inheriting from AbstractIntrusiveVariant");

public:
    // DEDUCTION ONLY

    template <class V>
    explicit IntrusiveVariantValue(const V&)
    {
        CORAL66_UNREACHABLE;
    }

    template <class V>
    explicit IntrusiveVariantValue(V&&) noexcept // NOLINT(*-forwarding-reference-overload)
    {
        CORAL66_UNREACHABLE;
    }

    template <class V, class... Args>
    explicit IntrusiveVariantValue(std::in_place_type_t<V>, Args&&...) noexcept
    {
        CORAL66_UNREACHABLE;
    }
};

template <class V>
IntrusiveVariantValue(const V&) -> IntrusiveVariantValue<typename V::base_type>;

template <class V>
IntrusiveVariantValue(V&&) -> IntrusiveVariantValue<typename V::base_type>;

template <class V, class... Args>
IntrusiveVariantValue(std::in_place_type_t<V>, Args&&...) -> IntrusiveVariantValue<typename V::base_type>;

template <class Base, class... SubClasses>
class IntrusiveVariantValue<Base, AbstractIntrusiveVariant<Base, SubClasses...>>
    : public detail::IntrusiveVariantStorage::IntrusiveVariantStorageMoveAss<
          Base, std::max({detail::IntrusiveVariantStorage::moveAss<SubClasses>()...}), SubClasses...>
{
    using detail::IntrusiveVariantStorage::IntrusiveVariantStorageBase<Base, SubClasses...>::get;

public:
    template <class T, class... Args>
    // ReSharper disable once CppNonExplicitConvertingConstructor
    requires(std::is_same_v<T, SubClasses> || ...) IntrusiveVariantValue(std::in_place_type_t<T>, Args&&... args)
    {
        std::construct_at(reinterpret_cast<T*>(this->m_storage), std::forward<Args>(args)...);
        constexpr auto var = Base::template indexOf<T>();
        this->m_index = var;
    }

    template <class T>
    requires(std::is_same_v<T, SubClasses> || ...)
        // ReSharper disable once CppNonExplicitConvertingConstructor
        IntrusiveVariantValue(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        std::construct_at(reinterpret_cast<T*>(this->m_storage), std::forward<T>(value));
        constexpr auto var = Base::template indexOf<T>();
        this->m_index = var;
    }

    template <class T>
    requires(std::is_same_v<T, SubClasses> || ...) IntrusiveVariantValue&
        operator=(T&& value) noexcept(std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>)
    {
        constexpr auto var = Base::template indexOf<T>();
        if (this->m_index == var)
        {
            this->get().template as<T>() = std::forward<T>(value);
        }
        else
        {
            this->destruct();
            std::construct_at(reinterpret_cast<T*>(this->m_storage), std::forward<T>(value));
            this->m_index = var;
        }
        return *this;
    }

    template <class T>
    requires(std::is_same_v<T, SubClasses> || ...)
        // ReSharper disable once CppNonExplicitConvertingConstructor
        IntrusiveVariantValue(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        std::construct_at(reinterpret_cast<T*>(this->m_storage), value);
        constexpr auto var = Base::template indexOf<T>();
        this->m_index = var;
    }

    template <class T>
    requires(std::is_same_v<T, SubClasses> || ...) IntrusiveVariantValue&
        operator=(const T& value) noexcept(std::is_nothrow_copy_assignable_v<T>
                                           && std::is_nothrow_copy_constructible_v<T>)
    {
        constexpr auto var = Base::template indexOf<T>();
        if (this->m_index == var)
        {
            this->get().template as<T>() = value;
        }
        else
        {
            this->destruct();
            std::construct_at(reinterpret_cast<T*>(this->m_storage), value);
            this->m_index = var;
        }
        return *this;
    }

    template <class U = Base>
    requires(std::is_same_v<U, Base> && (std::is_copy_constructible_v<SubClasses> && ...))
        // ReSharper disable once CppNonExplicitConvertingConstructor
        IntrusiveVariantValue(const U& value) noexcept((std::is_nothrow_copy_constructible_v<SubClasses> && ...))
    {
        this->m_index = value.index();
        constexpr std::array<void (*)(std::byte*, const Base&), sizeof...(SubClasses)> copyConstructors = {
            +[](std::byte* storage, const Base& value1)
            { std::construct_at(reinterpret_cast<SubClasses*>(storage), static_cast<const SubClasses&>(value1)); }...};
        copyConstructors[this->m_index](this->m_storage, value);
    }

    template <class U = Base>
    requires(
        std::is_same_v<U, Base>
        && (std::is_move_constructible_v<SubClasses>
            && ...)) explicit IntrusiveVariantValue(U&& value) noexcept((std::
                                                                             is_nothrow_move_constructible_v<SubClasses>
                                                                         && ...))
    {
        this->m_index = value.index();
        constexpr std::array<void (*)(std::byte*, Base&&), sizeof...(SubClasses)> moveConstructors = {
            +[](std::byte* storage, Base&& value1)
            { std::construct_at(reinterpret_cast<SubClasses*>(storage), static_cast<SubClasses&&>(value1)); }...};
        moveConstructors[this->m_index](this->m_storage, std::forward<U>(value));
    }

    template <class T, class... Args>
    requires(std::is_same_v<T, SubClasses> || ...) T& emplace(Args&&... args)
    {
        this->destruct();
        auto* object = std::construct_at(reinterpret_cast<T*>(this->m_storage), std::forward<Args>(args)...);
        constexpr auto var = Base::template indexOf<T>();
        this->m_index = var;
        return *object;
    }

    template <class Self>
    auto* CORAL66_NON_NULL operator->(this Self&& self)
    {
        return &std::forward<Self>(self).get();
    }

    template <class Self>
    auto* CORAL66_NON_NULL data(this Self&& self)
    {
        return &std::forward<Self>(self).get();
    }

    explicit operator const Base&() const&
    {
        return get();
    }

    template <class Self>
    decltype(auto) operator*(this Self&& self)
    {
        return std::forward<Self>(self).get();
    }
};

template <class T, class U>
requires EqualComparable<T, U> bool operator==(const IntrusiveVariantValue<T>& lhs, const IntrusiveVariantValue<U>& rhs)
{
    return *lhs == *rhs;
}

template <class T, class U>
requires EqualComparable<T, U> bool operator!=(const IntrusiveVariantValue<T>& lhs, const IntrusiveVariantValue<U>& rhs)
{
    return !(lhs == rhs);
}

template <class T, class U>
requires EqualComparable<T, U> bool operator==(const IntrusiveVariantValue<T>& lhs, const U& rhs)
{
    return *lhs == rhs;
}

template <class T, class U>
requires EqualComparable<T, U> bool operator==(const T& lhs, const IntrusiveVariantValue<U>& rhs)
{
    return lhs == *rhs;
}

template <class T, class U>
requires EqualComparable<T, U> bool operator!=(const T& lhs, const IntrusiveVariantValue<U>& rhs)
{
    return !(lhs == rhs);
}

template <class T, class U>
requires EqualComparable<T, U> bool operator!=(const IntrusiveVariantValue<T>& lhs, const U& rhs)
{
    return !(lhs == rhs);
}

} // namespace coral66
