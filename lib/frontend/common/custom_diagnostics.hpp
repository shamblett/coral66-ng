/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <type_traits>

#include "lib/support/intrusive_variant_value.hpp"
#include "lib/support/text.hpp"

namespace coral66
{
class source_interface;

namespace diagnostics
{
/**
 * Trait to convert a type T into a string for diagnostics.
 *
 * Specializations of this struct allow the diagnostic engine to format
 * custom types (like ConstantValue, or specific AST nodes) when they are passed
 * as arguments to a diagnostic message.
 */
template <class T>
struct StringConverter
{
    template <class... Ctx>
    static std::string inFormat(const T& arg, Ctx&&... /*ctx*/)
    {
        return coral66::to_string(arg);
    }

    template <class... Ctx>
    static std::string inArg(const T& arg, Ctx&&... /*ctx*/)
    {
        return coral66::to_string(arg);
    }
};

template <class T>
struct StringConverter<IntrusiveVariantValue<T>>
{
    template <class... Ctx>
    static std::string inFormat(const IntrusiveVariantValue<T>& arg, Ctx&&... ctx)
    {
        return StringConverter<T>::inFormat(arg, std::forward<Ctx>(ctx)...);
    }

    template <class... Ctx>
    static std::string inArg(const IntrusiveVariantValue<T>& arg, Ctx&&... ctx)
    {
        return StringConverter<T>::inArg(arg, std::forward<Ctx>(ctx)...);
    }
};

/**
 *  Handling for custom format specifiers in diagnostics.
 *
 * This struct allows defining custom behavior for format specifiers (e.g., %s)
 * based on the arguments provided.
 *
 * @tparam name The character sequence of the format specifier.
 */
template <char32_t... name>
struct CustomFormat;

template <>
struct CustomFormat<U's'>
{
    // Greedily accept everything so we can properly error
    template <class T>
    static std::string operator()(T integer)
    {
        static_assert(std::is_integral_v<std::decay_t<T>>, "Argument to %s must be an integer type");
        if (integer != 1)
        {
            return "s";
        }
        return "";
    }
};
} // namespace diagnostics
} // namespace coral66
