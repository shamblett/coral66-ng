/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <concepts>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

#include "utilities.hpp"

namespace coral66
{
namespace Constexpr
{
template <std::size_t... ints>
constexpr static auto integerSequenceToTuple(std::index_sequence<ints...>)
{
    return std::make_tuple(std::integral_constant<std::size_t, ints>{}...);
}

template <class T, std::size_t N>
class basic_fixed_string
{
public:
    T m_data[N] = {};

    template <class U, std::size_t M>
    constexpr basic_fixed_string(const U (&input)[M]) noexcept
    {
        for (std::size_t i = 0; i < N; i++)
        {
            CORAL66_ASSERT(input[i]);
            m_data[i] = input[i];
        }
    }

    constexpr std::basic_string_view<T> view() const noexcept
    {
        return {m_data, N};
    }

    static constexpr std::size_t size() noexcept
    {
        return N;
    }

    constexpr const T* begin() const noexcept
    {
        return m_data;
    }

    constexpr const T* end() const noexcept
    {
        return m_data + N;
    }

    constexpr T operator[](std::size_t i) const noexcept
    {
        return m_data[i];
    }
};

template <class T>
class basic_fixed_string<T, 0>
{
public:
    template <class U, std::size_t M>
    constexpr basic_fixed_string(const U (&)[M]) noexcept
    {
    }

    static constexpr std::basic_string_view<T> view() noexcept
    {
        return "";
    }

    static constexpr std::size_t size() noexcept
    {
        return 0;
    }

    static constexpr const T* begin() noexcept
    {
        return nullptr;
    }

    static constexpr const T* end() noexcept
    {
        return nullptr;
    }

    constexpr T operator[](std::size_t) const noexcept
    {
        CORAL66_UNREACHABLE;
    }
};

template <class U, std::size_t M>
basic_fixed_string(const U (&input)[M]) -> basic_fixed_string<U, M - 1>;

} // namespace Constexpr

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"

template <std::size_t max>
using suitableUInt =
    std::conditional_t<max <= std::numeric_limits<std::uint8_t>::max(), type_identity<std::uint8_t>,
                       std::conditional<max <= std::numeric_limits<std::uint16_t>::max(), std::uint16_t,
                                        std::conditional_t<max <= std::numeric_limits<std::uint32_t>::max(),
                                                           std::uint32_t, std::uint64_t>>>::type;

template <std::size_t max>
using suitableInt =
    std::conditional_t<max <= std::numeric_limits<std::int8_t>::max(), type_identity<std::int8_t>,
                       std::conditional<max <= std::numeric_limits<std::int16_t>::max(), std::int16_t,
                                        std::conditional_t<max <= std::numeric_limits<std::int32_t>::max(),
                                                           std::int32_t, std::int64_t>>>::type;

#pragma GCC diagnostic pop

template <class T>
concept TupleLike = requires
{
    typename std::tuple_size<T>::type;
};

template <class T>
struct is_tuple_impl : std::false_type
{
};
template <class... T>
struct is_tuple_impl<std::tuple<T...>> : std::true_type
{
};

template <class T>
concept Tuple = is_tuple_impl<T>::value;

template <class T>
struct is_variant_impl : std::false_type
{
};
template <class... T>
struct is_variant_impl<std::variant<T...>> : std::true_type
{
};

template <class T>
concept Variant = is_variant_impl<T>::value;

template <class T>
struct is_unique_ptr_impl : std::false_type
{
};
template <class T, class Del>
struct is_unique_ptr_impl<std::unique_ptr<T, Del>> : std::true_type
{
};

template <class T>
concept UniquePtr = is_unique_ptr_impl<T>::value;

template <class T>
struct is_optional_impl : std::false_type
{
};
template <class T>
struct is_optional_impl<std::optional<T>> : std::true_type
{
};

template <class T>
concept Optional = is_optional_impl<T>::value;

template <class T>
struct is_vector_impl : std::false_type
{
};
template <class T>
struct is_vector_impl<std::vector<T>> : std::true_type
{
};

template <class T>
concept Vector = is_vector_impl<T>::value;

template <class T>
struct is_shared_ptr_impl : std::false_type
{
};
template <class T>
struct is_shared_ptr_impl<std::shared_ptr<T>> : std::true_type
{
};

template <class T>
concept SharedPtr = is_shared_ptr_impl<T>::value;

template <class T>
concept SmartPtr = UniquePtr<T> || SharedPtr<T>;

template <class T, class U>
concept EqualComparable = requires(std::remove_cvref_t<T> const& a, std::remove_cvref_t<U> const& b)
{
    {a.operator==(b)}->std::convertible_to<bool>;
};

template <class T>
constexpr bool always_false = false;

template <class T>
constexpr bool always_true = true;

template <typename T>
concept CompleteType = requires
{
    sizeof(T);
};

namespace detail
{
template <class T, class... Args>
// ReSharper disable once CppFunctionIsNotImplemented
constexpr auto variantTypesContain(std::variant<Args...>) -> std::disjunction<std::is_same<T, Args>...>;

template <class VariantType, class OtherVariant>
struct VariantUnion;

template <class... ThisArgs, class First>
struct VariantUnion<std::variant<ThisArgs...>, std::variant<First>>
{
    using type = std::conditional_t<(std::is_same_v<ThisArgs, First> || ...), std::variant<ThisArgs...>,
                                    std::variant<ThisArgs..., First>>;
};

template <class... ThisArgs, class First, class... Args>
struct VariantUnion<std::variant<ThisArgs...>, std::variant<First, Args...>>
{
    using type = std::conditional_t<(std::is_same_v<ThisArgs, First> || ...),
                                    VariantUnion<std::variant<ThisArgs...>, std::variant<Args...>>,
                                    VariantUnion<std::variant<ThisArgs..., First>, std::variant<Args...>>>::type;
};

template <class VariantType>
struct Fold
{
    template <class OtherVariant>
    auto operator+(Fold<OtherVariant>) -> Fold<typename VariantUnion<VariantType, OtherVariant>::type>
    {
        CORAL66_UNREACHABLE;
    }

    using type = VariantType;
};

template <class... Variants>
// ReSharper disable once CppFunctionIsNotImplemented
constexpr auto variantTypeUnion(Variants...) -> decltype((std::declval<Fold<Variants>>() + ...))::type;
} // namespace detail

template <class T, class VariantType>
constexpr bool variantTypesContainV = decltype(detail::variantTypesContain<T>(std::declval<VariantType>())){};

template <class... VariantType>
using VariantUnion = decltype(detail::variantTypeUnion(std::declval<VariantType>()...));

#define CORAL66_CAT2(_1, _2) _1##_2
#define CORAL66_PASTE2(_1, _2) CORAL66_CAT2(_1, _2)
#define CORAL66_FOR(NAME, N, OP, FUNC, ...) CORAL66_PASTE2(P00_FOR, N)(NAME, OP, FUNC, __VA_ARGS__)

#define CORAL66_MACRO_COUNT_ARGUMENTS(...) CORAL66_MACRO_COUNT_ARGUMENTS_(__VA_ARGS__, CORAL66_MACRO_RSEQ_N())
#define CORAL66_MACRO_COUNT_ARGUMENTS_(...) CORAL66_MACRO_ARGUMENTS_N(__VA_ARGS__)
#define CORAL66_MACRO_ARGUMENTS_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,     \
                                  _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, \
                                  _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, \
                                  _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, N, ...)                       \
    N

#define CORAL66_MACRO_RSEQ_N()                                                                                        \
    63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36,   \
        35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, \
        7, 6, 5, 4, 3, 2, 1, 0

#define P00_FOR1(NAME, OP, FUNC, _1) FUNC(NAME, _1)
#define P00_FOR2(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR1(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR3(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR2(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR4(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR3(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR5(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR4(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR6(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR5(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR7(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR6(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR8(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR7(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR9(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR8(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR10(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR9(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR11(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR10(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR12(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR11(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR13(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR12(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR14(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR13(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR15(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR14(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR16(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR15(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR17(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR16(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR18(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR17(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR19(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR18(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR20(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR19(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR21(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR20(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR22(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR21(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR23(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR22(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR24(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR23(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR25(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR24(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR26(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR25(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR27(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR26(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR28(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR27(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR29(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR28(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR30(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR29(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR31(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR30(NAME, OP, FUNC, __VA_ARGS__)
#define P00_FOR32(NAME, OP, FUNC, _1, ...) FUNC(NAME, _1) OP P00_FOR31(NAME, OP, FUNC, __VA_ARGS__)

#define CORAL66_FOR_EACH(NAME, OP, FUNC, ...) \
    CORAL66_FOR(NAME, CORAL66_MACRO_COUNT_ARGUMENTS(__VA_ARGS__), OP, FUNC, __VA_ARGS__)

#define CORAL66_FOR_EACH_COMMA(NAME, FUNC, ...) \
    CORAL66_FOR(NAME, CORAL66_MACRO_COUNT_ARGUMENTS(__VA_ARGS__), , FUNC, __VA_ARGS__)

} // namespace coral66
