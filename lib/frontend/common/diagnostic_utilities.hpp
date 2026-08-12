/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

/**
 ** Diagnostics utilities
 *
 */

#include "lexer_common.hpp"
#include "lib/support/abstract_intrusive_variant.hpp"
#include "lib/support/utilities.hpp"

namespace coral66::diagnostics
{
namespace detail
{
template <class T>
concept IsIterable = requires(T t)
{
    std::begin(t);
    std::end(t);
};
} // namespace detail

struct PointLocation
{
    std::uint64_t offset;
    std::uint32_t fileId;
    std::uint32_t macroId;
};

using PointRange = std::pair<PointLocation, PointLocation>;

template <class T>
PointRange getPointRange(const T& arg)
{
    using U = std::decay_t<T>;
    if constexpr (std::is_same_v<PointRange, U>)
    {
        return arg;
    }
    else if constexpr (std::is_base_of_v<lexer::TokenBase, U>)
    {
        return {{.offset = arg.getOffset(), .fileId = arg.getFileId(), .macroId = arg.getMacroId()},
                {.offset = arg.getOffset() + arg.getLength(), .fileId = arg.getFileId(), .macroId = arg.getMacroId()}};
    }
    else if constexpr (std::is_convertible_v<U, std::uint64_t>)
    {
        return {{.offset = static_cast<std::uint64_t>(arg), .fileId = 0, .macroId = 0},
                {.offset = static_cast<std::uint64_t>(arg) + 1, .fileId = 0, .macroId = 0}};
    }
    else if constexpr (detail::IsIterable<T>)
    {
        CORAL66_ASSERT(std::begin(arg) != std::end(arg));
        auto& first = *std::begin(arg);
        auto& last = *(std::end(arg) - 1);
        if constexpr (std::is_pointer_v<std::decay_t<decltype(first)>> || SmartPtr<std::decay_t<decltype(first)>>)
        {
            return {getPointRange(*first).first, getPointRange(*last).second};
        }
        else
        {
            return {getPointRange(first).first, getPointRange(last).second};
        }
    }
    else if constexpr (Variant<U>)
    {
        return coral66::match(arg,
                              []<typename T0>(T0&& value)
                              {
                                  using V = std::decay_t<T0>;
                                  if constexpr (std::is_pointer_v<V> || SmartPtr<V>)
                                  {
                                      CORAL66_ASSERT(value);
                                      return getPointRange(*value);
                                  }
                                  else
                                  {
                                      return getPointRange(value);
                                  }
                              });
    }
    else if constexpr (std::tuple_size_v<U> == 2)
    {
        using T1 = std::decay_t<std::tuple_element_t<0, U>>;
        using T2 = std::decay_t<std::tuple_element_t<1, U>>;
        if constexpr (std::is_base_of_v<lexer::TokenBase, std::remove_pointer_t<T1>>
                      && std::is_base_of_v<lexer::TokenBase, std::remove_pointer_t<T2>>)
        {
            auto& [arg1, arg2] = arg;
            // ReSharper disable once CppDFAUnusedValue
            PointLocation first{};
            // ReSharper disable once CppDFAUnusedValue
            PointLocation second{};
            if constexpr (std::is_pointer_v<T1>)
            {
                CORAL66_ASSERT(arg1);
                first = {.offset = arg1->getOffset(), .fileId = arg1->getFileId(), .macroId = arg1->getMacroId()};
            }
            else
            {
                first = {.offset = arg1.getOffset(), .fileId = arg1.getFileId(), .macroId = arg1.getMacroId()};
            }
            if constexpr (std::is_pointer_v<T2>)
            {
                CORAL66_ASSERT(arg2);
                second = {.offset = arg2->getOffset() + arg2->getLength(),
                          .fileId = arg2->getFileId(),
                          .macroId = arg2->getMacroId()};
            }
            else
            {
                second = {.offset = arg2.getOffset() + arg2.getLength(),
                          .fileId = arg2.getFileId(),
                          .macroId = arg2.getMacroId()};
            }
            return {first, second};
        }
        else if constexpr (std::is_base_of_v<lexer::TokenBase, T1> && std::is_convertible_v<T2, std::uint64_t>)
        {
            auto& [arg1, arg2] = arg;
            return {
                {.offset = static_cast<std::uint64_t>(arg2), .fileId = arg1.getFileId(), .macroId = arg1.getMacroId()},
                {.offset = static_cast<std::uint64_t>(arg2) + 1,
                 .fileId = arg1.getFileId(),
                 .macroId = arg1.getMacroId()}};
        }
        else if constexpr (std::is_base_of_v<lexer::TokenBase, T2> && std::is_convertible_v<T1, std::uint64_t>)
        {
            auto& [arg1, arg2] = arg;
            return {
                {.offset = static_cast<std::uint64_t>(arg1), .fileId = arg2.getFileId(), .macroId = arg2.getMacroId()},
                {.offset = static_cast<std::uint64_t>(arg1) + 1,
                 .fileId = arg2.getFileId(),
                 .macroId = arg2.getMacroId()}};
        }
        else if constexpr (std::is_convertible_v<T1, std::uint64_t> && std::is_convertible_v<T2, std::uint64_t>)
        {
            auto& [arg1, arg2] = arg;
            return {{.offset = static_cast<std::uint64_t>(arg1), .fileId = 0, .macroId = 0},
                    {.offset = static_cast<std::uint64_t>(arg2), .fileId = 0, .macroId = 0}};
        }
        else
        {
            auto& [arg1, arg2] = arg;
            auto first = getPointRange(arg1);
            auto second = getPointRange(arg2);
            return {first.first, second.second};
        }
    }
    else
    {
        using T1 = std::decay_t<std::tuple_element_t<0, U>>;
        if constexpr (std::is_base_of_v<lexer::TokenBase, T1>)
        {
            auto& [arg1, arg2, arg3] = arg;
            return {
                {.offset = static_cast<std::uint64_t>(arg2), .fileId = arg1.getFileId(), .macroId = arg1.getMacroId()},
                {.offset = static_cast<std::uint64_t>(arg3), .fileId = arg1.getFileId(), .macroId = arg1.getMacroId()}};
        }
        else
        {
            auto& [arg1, arg2, arg3] = arg;
            return {
                {.offset = static_cast<std::uint64_t>(arg1), .fileId = arg3.getFileId(), .macroId = arg3.getMacroId()},
                {.offset = static_cast<std::uint64_t>(arg2), .fileId = arg3.getFileId(), .macroId = arg3.getMacroId()}};
        }
    }
}

} // namespace coral66::diagnostics
