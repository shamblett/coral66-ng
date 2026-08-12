/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include "lib/support/vendor/magic_enum/magic_enum.hpp"
#include "semantics.hpp"

namespace coral66::semantics
{

inline bool isUntyped(const Type& type)
{
    if (type.is<PrimitiveType>())
    {
        // No Untyped kind in PrimitiveType currently, assuming UntypedType class
        return false;
    }
    return type.is<UntypedType>();
}

inline bool isArray(const Type& type)
{
    return type.is<ArrayType>();
}

inline bool isInteger(const Type& type)
{
    if (auto* prim = type.tryAs<PrimitiveType>())
    {
        return prim->getKind() == PrimitiveType::Integer;
    }
    return false;
}

inline bool isArithmetic(const Type& type)
{
    if (auto* prim = type.tryAs<PrimitiveType>())
    {
        return prim->getKind() == PrimitiveType::Integer || prim->getKind() == PrimitiveType::Floating;
    }
    return type.is<FixedType>();
}

inline bool isScalar(const Type& type)
{
    return isArithmetic(type); // Pointers? Coral uses Location?
}

inline const char* primitiveTypeKindToString(const PrimitiveType::Kind kind)
{
    return magic_enum::enum_name(kind).data();
}

} // namespace coral66::semantics
