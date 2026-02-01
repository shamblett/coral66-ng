/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include "support/AbstractIntrusiveVariant.hpp"
#include "support/IntrusiveVariantValue.hpp"
#include "support/Util.hpp"

#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include <cstdint>
#include <tuple>

namespace coral66::Semantics
{

class ProgramInterface;

struct TableInfo;
struct OverlayInfo;

class PrimitiveType;
class ArrayType;
class ProcedureType;
class FixedType;
class TableType;
class OverlayType;
class VoidType;
class ErrorType;

// ============================================================================
// Type Hierarchy
// ============================================================================

class Type : public AbstractIntrusiveVariant<Type, PrimitiveType, ArrayType, ProcedureType,
                                             FixedType, TableType, OverlayType, VoidType, ErrorType>
{
public:
    enum TypeFlags : std::uint8_t
    {
        Nothing = 0,
        Value = 0b1,    // Passed by VALUE
        Location = 0b10 // Passed by LOCATION
    };

protected:
    TypeFlags m_flags;

    template <class T>
    Type(std::in_place_type_t<T>, TypeFlags flags)
        : AbstractIntrusiveVariant(std::in_place_type<T>), m_flags(static_cast<TypeFlags>(flags))
    {
    }

    Type(const Type&) = default;
    Type& operator=(const Type&) = default;
    Type(Type&&) noexcept = default;
    Type& operator=(Type&&) noexcept = default;

public:
    [[nodiscard]] bool isValue() const
    {
        return m_flags & Value;
    }

    [[nodiscard]] bool isLocation() const
    {
        return m_flags & Location;
    }

    [[nodiscard]] TypeFlags getFlags() const
    {
        return m_flags;
    }

    [[nodiscard]] bool isUndefined() const
    {
        return is<ErrorType>();
    }

    [[nodiscard]] bool operator==(const Type& rhs) const;

    [[nodiscard]] bool operator!=(const Type& rhs) const;

    [[nodiscard]] std::uint64_t getSizeOf(const ProgramInterface& program) const;

    [[nodiscard]] std::uint64_t getAlignOf(const ProgramInterface& program) const;
};

inline Type::TypeFlags operator|(const Type::TypeFlags lhs, Type::TypeFlags const rhs)
{
    using UT = std::underlying_type_t<Type::TypeFlags>;
    return static_cast<Type::TypeFlags>(static_cast<UT>(lhs) | static_cast<UT>(rhs));
}

inline Type::TypeFlags operator&(Type::TypeFlags const lhs, Type::TypeFlags const rhs)
{
    using UT = std::underlying_type_t<Type::TypeFlags>;
    return static_cast<Type::TypeFlags>(static_cast<UT>(lhs) & static_cast<UT>(rhs));
}

class PrimitiveType final : public Type
{
public:
    enum Kind : std::uint8_t
    {
        Integer,
        Floating,
        Bits // Part word
    };

private:
    Kind m_kind;

    friend class Type;

    [[nodiscard]] std::uint64_t getSizeOfImpl(const ProgramInterface&) const;
    [[nodiscard]] std::uint64_t getAlignOfImpl(const ProgramInterface&) const;
    [[nodiscard]] bool equalsImpl(const PrimitiveType& rhs) const
    {
        return m_kind == rhs.m_kind;
    }

public:
    PrimitiveType(Kind kind, TypeFlags flags = Nothing)
        : Type(std::in_place_type<PrimitiveType>, flags), m_kind(kind) {}

    [[nodiscard]] Kind getKind() const { return m_kind; }

    CLD_GEN_INTR_VAR_WARNING_FUNCS(PrimitiveType)
};

class VoidType final : public Type
{
    friend class Type;
    [[nodiscard]] static std::uint64_t getSizeOfImpl(const ProgramInterface&) { return 0; }
    [[nodiscard]] static std::uint64_t getAlignOfImpl(const ProgramInterface&) { return 0; }
    [[nodiscard]] static bool equalsImpl(const VoidType&) { return true; }
public:
    explicit VoidType(TypeFlags flags = Nothing) : Type(std::in_place_type<VoidType>, flags) {}
    CLD_GEN_INTR_VAR_WARNING_FUNCS(VoidType)
};

class FixedType final : public Type
{
    std::uint8_t m_totalBits;
    std::uint8_t m_fractionalBits;

    friend class Type;

    [[nodiscard]] std::uint64_t getSizeOfImpl(const ProgramInterface&) const;
    [[nodiscard]] std::uint64_t getAlignOfImpl(const ProgramInterface&) const;
    [[nodiscard]] bool equalsImpl(const FixedType& rhs) const
    {
        return std::tie(m_totalBits, m_fractionalBits) == std::tie(rhs.m_totalBits, rhs.m_fractionalBits);
    }

public:
    FixedType(std::uint8_t totalBits, std::uint8_t fractionalBits, TypeFlags flags = Nothing)
        : Type(std::in_place_type<FixedType>, flags), m_totalBits(totalBits), m_fractionalBits(fractionalBits) {}

    [[nodiscard]] std::uint8_t getTotalBits() const { return m_totalBits; }
    [[nodiscard]] std::uint8_t getFractionalBits() const { return m_fractionalBits; }

    CLD_GEN_INTR_VAR_WARNING_FUNCS(FixedType)
};

class ArrayType final : public Type
{
    const Type* m_elementType;
    std::vector<std::uint64_t> m_dimensions; 

    friend class Type;

    [[nodiscard]] std::uint64_t getSizeOfImpl(const ProgramInterface& program) const;
    [[nodiscard]] std::uint64_t getAlignOfImpl(const ProgramInterface& program) const;
    [[nodiscard]] bool equalsImpl(const ArrayType& rhs) const;

public:
    ArrayType(coral66::not_null<const Type> elementType, std::vector<std::uint64_t> dimensions, TypeFlags flags = Nothing)
        : Type(std::in_place_type<ArrayType>, flags), m_elementType(elementType), m_dimensions(std::move(dimensions)) {}

    [[nodiscard]] const Type& getElementType() const { return *m_elementType; }
    [[nodiscard]] const std::vector<std::uint64_t>& getDimensions() const { return m_dimensions; }

    CLD_GEN_INTR_VAR_WARNING_FUNCS(ArrayType)
};

class TableType final : public Type
{
    const TableInfo* m_info;
    friend class Type;
    [[nodiscard]] std::uint64_t getSizeOfImpl(const ProgramInterface& program) const;
    [[nodiscard]] std::uint64_t getAlignOfImpl(const ProgramInterface& program) const;
    [[nodiscard]] bool equalsImpl(const TableType& rhs) const;

public:
    explicit TableType(const TableInfo& info, TypeFlags flags = Nothing)
        : Type(std::in_place_type<TableType>, flags), m_info(&info) {}
    [[nodiscard]] const TableInfo& getInfo() const { return *m_info; }
    CLD_GEN_INTR_VAR_WARNING_FUNCS(TableType)
};

class OverlayType final : public Type
{
    const OverlayInfo* m_info;
    friend class Type;
    [[nodiscard]] std::uint64_t getSizeOfImpl(const ProgramInterface& program) const;
    [[nodiscard]] std::uint64_t getAlignOfImpl(const ProgramInterface& program) const;
    [[nodiscard]] bool equalsImpl(const OverlayType& rhs) const;

public:
    explicit OverlayType(const OverlayInfo& info, TypeFlags flags = Nothing)
        : Type(std::in_place_type<OverlayType>, flags), m_info(&info) {}
    [[nodiscard]] const OverlayInfo& getInfo() const { return *m_info; }
    CLD_GEN_INTR_VAR_WARNING_FUNCS(OverlayType)
};

class ProcedureType final : public Type
{
public:
    struct Parameter
    {
        const Type* CLD_NON_NULL type;
        std::string_view name;
        bool isValue; 
    };

private:
    const Type* m_returnType; 
    std::vector<Parameter> m_parameters;

    friend class Type;
    [[nodiscard]] static std::uint64_t getSizeOfImpl(const ProgramInterface&) { CLD_UNREACHABLE; }
    [[nodiscard]] static std::uint64_t getAlignOfImpl(const ProgramInterface&) { CLD_UNREACHABLE; }
    [[nodiscard]] bool equalsImpl(const ProcedureType& rhs) const;

public:
    ProcedureType(const Type* returnType, std::vector<Parameter> parameters, TypeFlags flags = Nothing)
        : Type(std::in_place_type<ProcedureType>, flags), m_returnType(returnType), m_parameters(std::move(parameters)) {}

    [[nodiscard]] const Type* getReturnType() const { return m_returnType; }
    [[nodiscard]] const std::vector<Parameter>& getParameters() const { return m_parameters; }

    CLD_GEN_INTR_VAR_WARNING_FUNCS(ProcedureType)
};

class ErrorType : public Type
{
    friend class Type;
    static std::size_t getSizeOfImpl(const ProgramInterface&) { CLD_UNREACHABLE; }
    static std::size_t getAlignOfImpl(const ProgramInterface&) { CLD_UNREACHABLE; }
    static bool equalsImpl(const ErrorType&) { return true; }
public:
    ErrorType(TypeFlags flags = Nothing) : Type(std::in_place_type<ErrorType>, flags) {}
    CLD_GEN_INTR_VAR_WARNING_FUNCS(ErrorType)
};

} // namespace coral66::Semantics
