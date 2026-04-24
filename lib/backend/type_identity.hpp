/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <algorithm>
#include <numeric>
#include <tuple>
#include <variant>

#include "lib/frontend/semantics/semantics.hpp"

/**
 *  TypeIdentity.hpp
 *  Defines backend-specific type hashing and equality comparison.
 *
 * This file contains TypeHasher and TypeEqual structures for semantics::Type.
 * Unlike the frontend's strict C type equality, the backend often needs to treat
 * qualified types (e.g., const int) as identical to their unqualified counterparts
 * (e.g., int) for the purpose of LLVM type generation and caching.
 */

namespace coral66::CGLLVM
{
// Default std::hash and operator== of semantics::Type and subclasses make use of equality the way C defines it. This
// does not equal the type system a backend may care about, however. The LLVM backend eg, does not care about any
// qualifiers, making nonqualified, const qualpragma once

template <class T>
struct TypeHasher
{
    std::size_t operator()(const T& arg) const noexcept
    {
        return std::hash<T>{}(arg);
    }
};

template <>
struct TypeHasher<semantics::Type>
{
    std::size_t operator()(const semantics::Type& arg) const noexcept;
};

template <>
struct TypeHasher<semantics::PrimitiveType>
{
    std::size_t operator()(const semantics::PrimitiveType& arg) const noexcept
    {
        return coral66::hashCombine(arg.getBitCount(), arg.isFloatingPoint());
    }
};

template <>
struct TypeHasher<semantics::ArrayType>
{
    std::size_t operator()(const semantics::ArrayType& arg) const noexcept
    {
        return coral66::hashCombine<TypeHasher>(arg.getSize(), arg.getType());
    }
};

template <>
struct TypeHasher<semantics::AbstractArrayType>
{
    std::size_t operator()(const semantics::AbstractArrayType& arg) const noexcept
    {
        return TypeHasher<semantics::Type>{}(arg.getType());
    }
};

template <>
struct TypeHasher<semantics::ValArrayType>
{
    std::size_t operator()(const semantics::ValArrayType& arg) const noexcept
    {
        return TypeHasher<semantics::Type>{}(arg.getType());
    }
};

template <>
struct TypeHasher<semantics::FunctionType>
{
    std::size_t operator()(const semantics::FunctionType& arg) const noexcept
    {
        const std::size_t hash =
            std::accumulate(arg.getParameters().begin(), arg.getParameters().end(), 0,
                            [](std::size_t hash, const semantics::FunctionType::Parameter& param)
                            { return coral66::rawHashCombine(hash, TypeHasher<semantics::Type>{}(*param.type)); });
        return coral66::rawHashCombine(hash, coral66::hashCombine<TypeHasher>(arg.isLastVararg(), arg.getReturnType()));
    }
};

template <>
struct TypeHasher<semantics::PointerType>
{
    std::size_t operator()(const semantics::PointerType& arg) const noexcept
    {
        return TypeHasher<semantics::Type>{}(arg.getElementType());
    }
};

template <>
struct TypeHasher<semantics::VectorType>
{
    std::size_t operator()(const semantics::VectorType& arg) const noexcept
    {
        return coral66::hashCombine<TypeHasher>(arg.getSize(), arg.getType());
    }
};

template <>
struct TypeHasher<semantics::FixedType>
{
    std::size_t operator()(const semantics::FixedType& arg) const noexcept
    {
        return coral66::hashCombine(arg.getTotalBits(), arg.getFractionalBits());
    }
};

template <>
struct TypeHasher<semantics::TableType>
{
    std::size_t operator()(const semantics::TableType& arg) const noexcept
    {
        // Using pointer identity for now
        return std::hash<const coral66::semantics::TableInfo*>{}(&arg.getInfo());
    }
};

template <>
struct TypeHasher<semantics::OverlayType>
{
    std::size_t operator()(const semantics::OverlayType& arg) const noexcept
    {
        // Using pointer identity for now
        return std::hash<const coral66::semantics::OverlayInfo*>{}(&arg.getInfo());
    }
};

inline std::size_t TypeHasher<semantics::Type>::operator()(const semantics::Type& arg) const noexcept
{
    return coral66::rawHashCombine(std::hash<std::size_t>{}(arg.index()),
                                   arg.match(
                                       []<typename T0>(const T0& arg1) -> std::size_t
                                       {
                                           using T = std::decay_t<T0>;
                                           return TypeHasher<T>{}(arg1);
                                       }));
}

template <class T>
struct TypeEqual;

template <class... Args>
struct TypeEqual<std::variant<Args...>>
{
    bool operator()(const std::variant<Args...>& lhs, const std::variant<Args...>& rhs) const noexcept
    {
        if (lhs.index() != rhs.index())
        {
            return false;
        }
        return coral66::match(lhs,
                              [&]<typename T0>(T0&& value)
                              {
                                  using T = std::decay_t<T0>;
                                  return TypeEqual<T>{}(value, coral66::get<T>(rhs));
                              });
    }
};

template <>
struct TypeEqual<semantics::Type>
{
    bool operator()(const semantics::Type& lhs, const semantics::Type& rhs) const noexcept;
};

template <>
struct TypeEqual<semantics::PrimitiveType>
{
    bool operator()(const semantics::PrimitiveType& lhs, const semantics::PrimitiveType& rhs) const noexcept
    {
        return std::tuple(lhs.getBitCount(), lhs.isFloatingPoint())
               == std::tuple(rhs.getBitCount(), rhs.isFloatingPoint());
    }
};

template <>
struct TypeEqual<semantics::ArrayType>
{
    bool operator()(const semantics::ArrayType& lhs, const semantics::ArrayType& rhs) const noexcept
    {
        if (lhs.getSize() != rhs.getSize())
        {
            return false;
        }
        return TypeEqual<semantics::Type>{}(lhs.getType(), rhs.getType());
    }
};

template <>
struct TypeEqual<semantics::AbstractArrayType>
{
    bool operator()(const semantics::AbstractArrayType& lhs, const semantics::AbstractArrayType& rhs) const noexcept
    {
        return TypeEqual<semantics::Type>{}(lhs.getType(), rhs.getType());
    }
};

template <>
struct TypeEqual<semantics::ValArrayType>
{
    bool operator()(const semantics::ValArrayType& lhs, const semantics::ValArrayType& rhs) const noexcept
    {
        return TypeEqual<semantics::Type>{}(lhs.getType(), rhs.getType());
    }
};

template <>
struct TypeEqual<semantics::PointerType>
{
    bool operator()(const semantics::PointerType& lhs, const semantics::PointerType& rhs) const noexcept
    {
        return TypeEqual<semantics::Type>{}(lhs.getElementType(), rhs.getElementType());
    }
};

template <>
struct TypeEqual<semantics::FunctionType>
{
    bool operator()(const semantics::FunctionType& lhs, const semantics::FunctionType& rhs) const noexcept
    {
        if (lhs.isLastVararg() != rhs.isLastVararg())
        {
            return false;
        }
        if (!TypeEqual<semantics::Type>{}(lhs.getReturnType(), rhs.getReturnType()))
        {
            return false;
        }
        return std::ranges::equal(
            lhs.getParameters(), rhs.getParameters(),
            [](const semantics::FunctionType::Parameter& lhs, const semantics::FunctionType::Parameter& rhs)
            { return TypeEqual<semantics::Type>{}(*lhs.type, *rhs.type); });
    }
};

template <>
struct TypeEqual<semantics::StructType>
{
    bool operator()(const semantics::StructType& lhs, const semantics::StructType& rhs) const noexcept
    {
        return &lhs.getInfo() == &rhs.getInfo();
    }
};

template <>
struct TypeEqual<semantics::UnionType>
{
    bool operator()(const semantics::UnionType& lhs, const semantics::UnionType& rhs) const noexcept
    {
        return &lhs.getInfo() == &rhs.getInfo();
    }
};

template <>
struct TypeEqual<semantics::EnumType>
{
    bool operator()(const semantics::EnumType& lhs, const semantics::EnumType& rhs) const noexcept
    {
        return &lhs.getInfo() == &rhs.getInfo();
    }
};

template <>
struct TypeEqual<semantics::VectorType>
{
    bool operator()(const semantics::VectorType& lhs, const semantics::VectorType& rhs) const noexcept
    {
        if (lhs.getSize() != rhs.getSize())
        {
            return false;
        }
        return TypeEqual<semantics::Type>{}(lhs.getType(), rhs.getType());
    }
};

template <>
struct TypeEqual<semantics::FixedType>
{
    bool operator()(const semantics::FixedType& lhs, const semantics::FixedType& rhs) const noexcept
    {
        return std::tuple(lhs.getTotalBits(), lhs.getFractionalBits())
               == std::tuple(rhs.getTotalBits(), rhs.getFractionalBits());
    }
};

template <>
struct TypeEqual<semantics::TableType>
{
    bool operator()(const semantics::TableType& lhs, const semantics::TableType& rhs) const noexcept
    {
        return &lhs.getInfo() == &rhs.getInfo();
    }
};

template <>
struct TypeEqual<semantics::OverlayType>
{
    bool operator()(const semantics::OverlayType& lhs, const semantics::OverlayType& rhs) const noexcept
    {
        return &lhs.getInfo() == &rhs.getInfo();
    }
};

template <>
struct TypeEqual<semantics::ErrorType>
{
    bool operator()(const semantics::ErrorType&, const semantics::ErrorType&) const noexcept
    {
        return true;
    }
};

} // namespace coral66::CGLLVM
